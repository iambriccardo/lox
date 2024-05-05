#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "compiler.h"
#include "object.h"
#include "scanner.h"
#include "value.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
  Token current;
  Token previous;
  bool hadError;
  bool panicMode;
} Parser;

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,    // =
  PREC_QUESTION_MARK, // ?
  PREC_COLON,         // :
  PREC_OR,            // or
  PREC_AND,           // and
  PREC_EQUALITY,      // == !=
  PREC_COMPARISON,    // < > <= >=
  PREC_TERM,          // + -
  PREC_FACTOR,        // * /
  PREC_UNARY,         // ! -
  PREC_CALL,          // . ()
  PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

typedef struct {
  Token name;
  int depth;
} Local;

typedef enum { TYPE_FUNCTION, TYPE_SCRIPT } FunctionType;

typedef struct {
  struct Compiler *enclosing;
  ObjFunction *function;
  FunctionType type;

  Local locals[UINT8_COUNT];
  int localCount;
  int scopeDepth;
} Compiler;

typedef enum { BREAK, CONTINUE } InterruptorType;

typedef enum {
  NONE,
  SWITCH_STATEMENT,
  WHILE_STATEMENT,
  FOR_STATEMENT,
  BLOCK_STATEMENT,
  IF_STATEMENT
} EnclosingContext;

typedef struct {
  InterruptorType type;
  int position;
} Interruptor;

typedef struct {
  Interruptor interruptors[UINT8_COUNT];
  int count;
} Interruptors;

#define BREAK_INTERRUPTOR(p) ((Interruptor){.type = BREAK, .position = p})
#define CONTINUE_INTERRUPTOR(p) ((Interruptor){.type = CONTINUE, .position = p})

#define BUILD_INTERRUPTORS(i, c) ((Interruptors){.interruptors = i, .count = c})
#define NO_INTERRUPTORS BUILD_INTERRUPTORS({}, 0)

Parser parser;
Compiler *current = NULL;

int enclosingContextsCount = 0;
EnclosingContext enclosingContexts[UINT8_COUNT];

static Chunk *currentChunk() { return &current->function->chunk; }

static void errorAt(Token *token, const char *message) {
  if (parser.panicMode)
    return;

  parser.panicMode = true;
  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    // Nothing.
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.hadError = true;
}

static void errorAtCurrent(const char *message) {
  errorAt(&parser.current, message);
}

static void advance() {
  parser.previous = parser.current;

  for (;;) {
    parser.current = scanToken();
    if (parser.current.type != TOKEN_ERROR)
      break;

    errorAtCurrent(parser.current.start);
  }
}

static bool check(TokenType type) { return parser.current.type == type; }

static void consume(TokenType type, const char *message) {
  if (check(type)) {
    advance();
    return;
  }

  errorAtCurrent(message);
}

static bool match(TokenType type) {
  if (!check(type))
    return false;
  advance();
  return true;
}

static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}

static int emitLoop(int loopStart) {
  emitByte(OP_LOOP);

  int offset = currentChunk()->count - loopStart + 2;
  if (offset > UINT16_MAX) {
    errorAtCurrent("Loop body too large.");
  }

  if (loopStart == -1) {
    emitByte(0xff);
    emitByte(0xff);
  } else {
    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
  }

  return currentChunk()->count - 2;
}

static int emitJump(uint8_t instruction) {
  emitByte(instruction);
  emitByte(0xff);
  emitByte(0xff);

  return currentChunk()->count - 2;
}

static void emitReturn() {
  emitByte(OP_NIL);
  emitByte(OP_RETURN);
}

static uint8_t makeConstant(Value value) {
  int constant = addConstant(currentChunk(), value);
  if (constant > UINT8_MAX) {
    errorAtCurrent("Too many constants in one chunk.");
    return 0;
  }

  return (uint8_t)constant;
}

static void emitConstant(Value value) {
  emitBytes(OP_CONSTANT, makeConstant(value));
}

static void patchJump(int offset) {
  // -2 to adjust for the bytecode for the jump offset itself.
  int jump = currentChunk()->count - offset - 2;

  if (jump > UINT16_MAX) {
    errorAtCurrent("Too much code to jump over.");
  }

  currentChunk()->code[offset] = (jump >> 8) & 0xff;
  currentChunk()->code[offset + 1] = jump & 0xff;
}

static void patchLoop(int offset, int loopStart) {
  // -2 to adjust for the bytecode for the jump offset itself.
  int jump = offset - loopStart + 2;

  if (jump > UINT16_MAX) {
    errorAtCurrent("Too much code to loop over.");
  }

  currentChunk()->code[offset] = (jump >> 8) & 0xff;
  currentChunk()->code[offset + 1] = jump & 0xff;
}

static void initCompiler(Compiler *compiler, FunctionType type) {
  compiler->enclosing = current;
  compiler->function = NULL;
  compiler->type = type;
  compiler->localCount = 0;
  compiler->scopeDepth = 0;
  compiler->function = newFunction();
  current = compiler;
  if (type != TYPE_SCRIPT) {
    current->function->name =
        copyString(parser.previous.start, parser.previous.length);
  }

  Local *local = &current->locals[current->localCount++];
  local->depth = 0;
  local->name.start = "";
  local->name.length = 0;
}

static ObjFunction *endCompiler() {
  emitReturn();
  ObjFunction *function = current->function;

#ifdef DEBUG_PRINT_CODE
  if (!parser.hadError) {
    disassembleChunk(currentChunk(), function->name != NULL
                                         ? function->name->chars
                                         : "<script>");
  }
#endif

  current = current->enclosing;
  return function;
}

static void beginScope() { current->scopeDepth++; }

static void endScope() {
  current->scopeDepth--;

  while (current->localCount > 0 &&
         current->locals[current->localCount - 1].depth > current->scopeDepth) {
    emitByte(OP_POP);
    current->localCount--;
  }
}

static Interruptors mergeInterruptors(Interruptors a, Interruptors b) {
  for (int i = 0; i < b.count; i++) {
    a.interruptors[a.count++] = b.interruptors[i];
  }
  return a;
}

static void addEnclosingContext(EnclosingContext enclosingContext) {
  enclosingContexts[enclosingContextsCount++] = enclosingContext;
}

static void removeEnclosingContext() {
  if (enclosingContextsCount == 0) {
    return;
  }

  if (enclosingContextsCount == UINT8_COUNT) {
    errorAtCurrent("Too many nested contexts");
    return;
  }

  enclosingContexts[enclosingContextsCount - 1] = NONE;
  enclosingContextsCount--;
}

static void validateInterruptor(InterruptorType interruptorType) {
  for (int i = enclosingContextsCount - 1; i >= 0; i--) {
    EnclosingContext enclosingContext = enclosingContexts[i];
    if (interruptorType == BREAK && (enclosingContext == FOR_STATEMENT ||
                                     enclosingContext == WHILE_STATEMENT ||
                                     enclosingContext == SWITCH_STATEMENT)) {
      return;
    } else if (interruptorType == CONTINUE &&
               (enclosingContext == FOR_STATEMENT ||
                enclosingContext == WHILE_STATEMENT)) {
      return;
    }
  }

  if (interruptorType == BREAK) {
    errorAtCurrent("The 'break' statement can't be defined here");
  } else if (interruptorType == CONTINUE) {
    errorAtCurrent("The 'continue' statement can't be defined here");
  }
}

static void unwindEnclosingContexts(InterruptorType interruptorType) {
  for (int i = enclosingContextsCount - 1; i >= 0; i--) {
    EnclosingContext enclosingContext = enclosingContexts[i];
    if (enclosingContext == BLOCK_STATEMENT) {
      // We remove all locals at this depth.
      while (current->localCount > 0 &&
             current->locals[current->localCount - 1].depth >=
                 current->scopeDepth) {
        emitByte(OP_POP);
      }
    } else {
      // This is an edge case, since the 'continue' statement doesn't refer to a
      // switch statement but rahter to the innermost 'while' or 'for' loop and
      // since we do not jump to the end of the statement where it is
      // destructed, we want to pop the condition of the 'switch' statement.
      if (enclosingContext == SWITCH_STATEMENT && interruptorType == CONTINUE) {
        emitByte(OP_POP); // 'switch' condition.
      }

      break;
    }
  };
}

static void expression();
static Interruptors statement();
static Interruptors declaration();
static ParseRule *getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static uint8_t identifierConstant(Token *name) {
  return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

static bool identifiersEqual(Token *a, Token *b) {
  if (a->length != b->length)
    return false;
  return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(Compiler *compiler, Token *name) {
  for (int i = compiler->localCount - 1; i >= 0; i--) {
    Local *local = &compiler->locals[i];
    if (identifiersEqual(name, &local->name)) {
      if (local->depth == -1) {
        errorAtCurrent("Can't read local variable in its own initializer.");
      }
      return i;
    }
  }

  return -1;
}

static void addLocal(Token name) {
  if (current->localCount == UINT8_COUNT) {
    errorAtCurrent("Too many local variables in function.");
    return;
  }

  Local *local = &current->locals[current->localCount++];
  local->name = name;
  local->depth = -1;
}

static void declareVariable() {
  if (current->scopeDepth == 0) {
    return;
  }

  Token *name = &parser.previous;
  for (int i = current->localCount - 1; i >= 0; i--) {
    Local *local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scopeDepth) {
      break;
    }

    if (identifiersEqual(name, &local->name)) {
      errorAtCurrent("Already a variable with this name in this scope.");
    }
  }

  addLocal(*name);
}

static uint8_t parseVariable(const char *errorMessage) {
  consume(TOKEN_IDENTIFIER, errorMessage);

  declareVariable();
  if (current->scopeDepth > 0) {
    return 0;
  }

  return identifierConstant(&parser.previous);
}

static void markInitialized() {
  if (current->scopeDepth == 0) {
    return;
  }
  current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable(uint8_t global) {
  if (current->scopeDepth > 0) {
    markInitialized();
    return;
  }

  emitBytes(OP_DEFINE_GLOBAL, global);
}

static uint8_t argumentList() {
  uint8_t argCount = 0;
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      expression();
      if (argCount == 255) {
        errorAtCurrent("Can't have more than 255 arguments.");
      }
      argCount++;
    } while (match(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
  return argCount;
}

static void and_(bool canAssign) {
  int endJump = emitJump(OP_JUMP_IF_FALSE);

  emitByte(OP_POP);
  parsePrecedence(PREC_AND);

  patchJump(endJump);
}

static void ternary(bool canAssign) {
  TokenType operatorType = parser.previous.type;
  ParseRule *rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule->precedence + 1));
  advance();
  parsePrecedence((Precedence)(rule->precedence + 1));

  // TODO: we need to emit a ternary operation here, based on the operator type.
  printf("EMITTING TERNARY");
}

static void binary(bool canAssign) {
  TokenType operatorType = parser.previous.type;
  ParseRule *rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule->precedence + 1));

  switch (operatorType) {
  case TOKEN_BANG_EQUAL:
    emitBytes(OP_EQUAL, OP_NOT);
    break;
  case TOKEN_EQUAL_EQUAL:
    emitByte(OP_EQUAL);
    break;
  case TOKEN_GREATER:
    emitByte(OP_GREATER);
    break;
  case TOKEN_GREATER_EQUAL:
    emitBytes(OP_LESS, OP_NOT);
    break;
  case TOKEN_LESS:
    emitByte(OP_LESS);
    break;
  case TOKEN_LESS_EQUAL:
    emitBytes(OP_GREATER, OP_NOT);
    break;
  case TOKEN_PLUS:
    emitByte(OP_ADD);
    break;
  case TOKEN_MINUS:
    emitBytes(OP_NEGATE, OP_ADD);
    break;
  case TOKEN_STAR:
    emitByte(OP_MULTIPLY);
    break;
  case TOKEN_SLASH:
    emitByte(OP_DIVIDE);
    break;
  default:
    return; // Unreachable.
  }
}

static void call(bool canAssign) {
  uint8_t argCount = argumentList();
  emitBytes(OP_CALL, argCount);
}

static void literal(bool canAssign) {
  switch (parser.previous.type) {
  case TOKEN_FALSE:
    emitByte(OP_FALSE);
    break;
  case TOKEN_NIL:
    emitByte(OP_NIL);
    break;
  case TOKEN_TRUE:
    emitByte(OP_TRUE);
    break;
  default:
    return; // Unreachable.
  }
}

static void grouping(bool canAssign) {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(bool canAssign) {
  double value = strtod(parser.previous.start, NULL);
  emitConstant(NUMBER_VAL(value));
}

static void or_(bool canAssign) {
  int elseJump = emitJump(OP_JUMP_IF_FALSE);
  int endJump = emitJump(OP_JUMP);

  patchJump(elseJump);
  emitByte(OP_POP);

  parsePrecedence(PREC_OR);
  patchJump(endJump);
}

static void string(bool canAssign) {
  emitConstant(OBJ_VAL(
      referenceString(parser.previous.start + 1, parser.previous.length - 2)));
}

static void namedVariable(Token name, bool canAssign) {
  uint8_t getOp, setOp;
  int arg = resolveLocal(current, &name);
  if (arg != -1) {
    getOp = OP_GET_LOCAL;
    setOp = OP_SET_LOCAL;
  } else {
    arg = identifierConstant(&name);
    getOp = OP_GET_GLOBAL;
    setOp = OP_SET_GLOBAL;
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(setOp, (uint8_t)arg);
  } else {
    emitBytes(getOp, (uint8_t)arg);
  }
}

static void variable(bool canAssign) {
  namedVariable(parser.previous, canAssign);
}

static void unary(bool canAssign) {
  TokenType operatorType = parser.previous.type;

  // Compile the operand.
  parsePrecedence(PREC_UNARY);

  // Emit the operator instruction.
  switch (operatorType) {
  case TOKEN_BANG:
    emitByte(OP_NOT);
    break;
  case TOKEN_MINUS:
    emitByte(OP_NEGATE);
    break;
  default:
    return; // Unreachable.
  }
}

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, call, PREC_CALL},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_COLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_QUESTION_MARK] = {NULL, ternary, PREC_QUESTION_MARK},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, and_, PREC_AND},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, or_, PREC_OR},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
    [TOKEN_BREAK] = {NULL, NULL, PREC_NONE},
    [TOKEN_CONTINUE] = {NULL, NULL, PREC_NONE},
};

static void expression() { parsePrecedence(PREC_ASSIGNMENT); }

static Interruptors block() {
  Interruptors interruptors = NO_INTERRUPTORS;

  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    interruptors = mergeInterruptors(interruptors, declaration());
  }

  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");

  // The block statement doesn't "capture" any interruptors, thus we forward
  // them upstream.
  return interruptors;
}

static void function(FunctionType type) {
  Compiler compiler;
  initCompiler(&compiler, type);
  beginScope();

  consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      current->function->arity++;
      if (current->function->arity > 255) {
        errorAtCurrent("Can't have more than 255 parameters.");
      }
      uint8_t constant = parseVariable("Expect parameter name.");
      defineVariable(constant);
    } while (match(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
  consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
  block();

  ObjFunction *function = endCompiler();
  emitBytes(OP_CONSTANT, makeConstant(OBJ_VAL(function)));
}

static void funDeclaration() {
  uint8_t global = parseVariable("Expect function name.");
  markInitialized();
  function(TYPE_FUNCTION);
  defineVariable(global);
}

static void varDeclaration() {
  uint8_t global = parseVariable("Expect variable name.");

  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emitByte(OP_NIL);
  }
  consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

  defineVariable(global);
}

static void expressionStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
  emitByte(OP_POP);
}

static Interruptors switchCaseExpression(bool is_default_case) {
  Interruptors interruptors = NO_INTERRUPTORS;

  if (!is_default_case) {
    expression();
  }
  consume(TOKEN_COLON, "Expect ':' after switch case expression.");

  if (!is_default_case) {
    emitByte(OP_SWITCH_CASE_EQUAL);
    int nextCaseJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); // Condition.
    emitByte(OP_POP); // Case expression.

    interruptors = statement();

    int skipJump = emitJump(OP_JUMP);
    patchJump(nextCaseJump);
    emitByte(OP_POP); // Condition.
    emitByte(OP_POP); // Case expression.

    patchJump(skipJump);
  } else {
    interruptors = statement();
  }

  return interruptors;
}

static Interruptors switchStatement() {
  Interruptors interruptors = NO_INTERRUPTORS;

  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'switch'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  consume(TOKEN_LEFT_BRACE, "Expect '{' after 'switch' condition.");

  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    if (match(TOKEN_CASE)) {
      interruptors =
          mergeInterruptors(interruptors, switchCaseExpression(false));
    } else if (match(TOKEN_DEFAULT)) {
      interruptors =
          mergeInterruptors(interruptors, switchCaseExpression(true));
    }
  }

  consume(TOKEN_RIGHT_BRACE, "Expect '}' after 'switch' statement.");

  // We forward the 'continue' interruptor upstream since it's not supported by
  // the 'switch' statement.
  Interruptors unsupportedInterruptors = NO_INTERRUPTORS;
  for (int i = 0; i < interruptors.count; i++) {
    Interruptor interruptor = interruptors.interruptors[i];
    if (interruptor.type == BREAK) {
      patchJump(interruptor.position);
    } else if (interruptor.type == CONTINUE) {
      unsupportedInterruptors.interruptors[unsupportedInterruptors.count++] =
          interruptor;
    }
  }

  emitByte(OP_POP); // Condition.

  return unsupportedInterruptors;
}

static void forStatement() {
  beginScope();
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
  if (match(TOKEN_SEMICOLON)) {
    // No initializer.
  } else if (match(TOKEN_VAR)) {
    varDeclaration();
  } else {
    expressionStatement();
  }

  int loopStart = currentChunk()->count;
  int exitJump = -1;
  if (!match(TOKEN_SEMICOLON)) {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

    // Jump out of the loop if the condition is false.
    exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); // Condition.
  }

  if (!match(TOKEN_RIGHT_PAREN)) {
    int bodyJump = emitJump(OP_JUMP);
    int incrementStart = currentChunk()->count;
    expression();
    emitByte(OP_POP);
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

    emitLoop(loopStart);
    loopStart = incrementStart;
    patchJump(bodyJump);
  } else {
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");
  }

  Interruptors interruptors = statement();
  emitLoop(loopStart);

  // We now patch all the jumps and loops that were defined via interruptor
  // statements.
  for (int i = 0; i < interruptors.count; i++) {
    Interruptor interruptor = interruptors.interruptors[i];
    if (interruptor.type == BREAK) {
      patchJump(interruptor.position);
    } else if (interruptor.type == CONTINUE) {
      patchLoop(interruptor.position, loopStart);
    }
  }

  if (exitJump != -1) {
    patchJump(exitJump);
    emitByte(OP_POP); // Condition.
  }

  endScope();
}

static Interruptors ifStatement() {
  Interruptors interruptors = NO_INTERRUPTORS;

  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  int thenJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP); // Condition.

  interruptors = statement();

  int elseJump = emitJump(OP_JUMP);

  patchJump(thenJump);
  emitByte(OP_POP); // Condition.

  if (match(TOKEN_ELSE)) {
    interruptors = mergeInterruptors(interruptors, statement());
  }
  patchJump(elseJump);

  // The 'if' statement doesn't "capture" any interruptors, thus we forward them
  // upstream.
  return interruptors;
}

static void printStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value.");
  emitByte(OP_PRINT);
}

static void returnStatement() {
  if (current->type == TYPE_SCRIPT) {
    errorAtCurrent("Can't return from top-level code.");
  }

  if (match(TOKEN_SEMICOLON)) {
    emitReturn();
  } else {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
    emitByte(OP_RETURN);
  }
}

static void whileStatement() {
  int loopStart = currentChunk()->count;
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  int exitJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP); // Condition.

  Interruptors interruptors = statement();
  emitLoop(loopStart);

  // We now patch all the jumps and loops that were defined via interruptor
  // statements.
  for (int i = 0; i < interruptors.count; i++) {
    Interruptor interruptor = interruptors.interruptors[i];
    if (interruptor.type == BREAK) {
      patchJump(interruptor.position);
    } else if (interruptor.type == CONTINUE) {
      patchLoop(interruptor.position, loopStart);
    }
  }

  patchJump(exitJump);
  emitByte(OP_POP); // Condition.
}

static void synchronize() {
  parser.panicMode = false;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON)
      return;

    switch (parser.current.type) {
    case TOKEN_SWITCH:
    case TOKEN_CLASS:
    case TOKEN_FUN:
    case TOKEN_VAR:
    case TOKEN_FOR:
    case TOKEN_IF:
    case TOKEN_WHILE:
    case TOKEN_PRINT:
    case TOKEN_RETURN:
      return;

    default:; // Do nothing.
    }

    advance();
  }
}

static Interruptors declaration() {
  Interruptors interruptors = NO_INTERRUPTORS;
  if (match(TOKEN_FUN)) {
    funDeclaration();
  } else if (match(TOKEN_VAR)) {
    varDeclaration();
  } else {
    interruptors = statement();
  }

  if (parser.panicMode) {
    synchronize();
  }

  return interruptors;
}

static Interruptors interruptorStatement(InterruptorType interruptorType) {
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");

  // We check whether the interruptor can be defined consider the enclosing
  // contexts.
  validateInterruptor(interruptorType);

  // For each enclosing context, we emit the byte code necessary to correclty
  // unwind.
  unwindEnclosingContexts(interruptorType);

  Interruptors interruptors = NO_INTERRUPTORS;
  if (interruptorType == BREAK) {
    interruptors =
        BUILD_INTERRUPTORS({BREAK_INTERRUPTOR(emitJump(OP_JUMP))}, 1);
  } else if (interruptorType == CONTINUE) {
    interruptors = BUILD_INTERRUPTORS({CONTINUE_INTERRUPTOR(emitLoop(-1))}, 1);
  }

  return interruptors;
}

static Interruptors statement() {
  Interruptors interruptors = NO_INTERRUPTORS;

  if (match(TOKEN_PRINT)) {
    printStatement();
  } else if (match(TOKEN_FOR)) {
    addEnclosingContext(FOR_STATEMENT);
    forStatement();
    removeEnclosingContext();
  } else if (match(TOKEN_IF)) {
    addEnclosingContext(IF_STATEMENT);
    interruptors = ifStatement();
    removeEnclosingContext();
  } else if (match(TOKEN_RETURN)) {
    returnStatement();
  } else if (match(TOKEN_WHILE)) {
    addEnclosingContext(WHILE_STATEMENT);
    whileStatement();
    removeEnclosingContext();
  } else if (match(TOKEN_LEFT_BRACE)) {
    beginScope();
    addEnclosingContext(BLOCK_STATEMENT);
    interruptors = block();
    removeEnclosingContext();
    endScope();
  } else if (match(TOKEN_SWITCH)) {
    addEnclosingContext(SWITCH_STATEMENT);
    interruptors = switchStatement();
    removeEnclosingContext();
  } else if (match(TOKEN_BREAK)) {
    interruptors = interruptorStatement(BREAK);
  } else if (match(TOKEN_CONTINUE)) {
    interruptors = interruptorStatement(CONTINUE);
  } else {
    expressionStatement();
  }

  return interruptors;
}

static ParseRule *getRule(TokenType type) { return &rules[type]; }

static void parsePrecedence(Precedence precedence) {
  advance();
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if (prefixRule == NULL) {
    errorAtCurrent("Expect expression.");
    return;
  }

  bool canAssign = precedence <= PREC_ASSIGNMENT;
  prefixRule(canAssign);

  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule(canAssign);
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    errorAtCurrent("Invalid assignment target.");
  }
}

ObjFunction *compile(const char *source) {
  initScanner(source);

  Compiler compiler;
  initCompiler(&compiler, TYPE_SCRIPT);

  parser.hadError = false;
  parser.panicMode = false;

  advance();

  while (!match(TOKEN_EOF)) {
    declaration();
  }

  ObjFunction *function = endCompiler();
  return parser.hadError ? NULL : function;
}
