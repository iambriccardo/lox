#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
  OP_CONSTANT,
  OP_CONSTANT_LONG,
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_POP,
  OP_GET_LOCAL,
  OP_GET_GLOBAL,
  OP_DEFINE_GLOBAL,
  OP_SET_LOCAL,
  OP_SET_GLOBAL,
  OP_SWITCH_CASE_EQUAL,
  OP_GET_UPVALUE,
  OP_SET_UPVALUE,
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
  OP_ADD,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NOT,
  OP_NEGATE,
  OP_PRINT,
  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_LOOP,
  OP_CALL,
  OP_CLOSURE,
  OP_RETURN,
} OpCode;

typedef struct {
  uint8_t code_start;
  uint8_t code_end;
  int lineno;
} LineRange;

typedef struct {
  int count;
  int capacity;
  uint8_t *code;
  int line_ranges_count;
  LineRange *line_ranges;
  ValueArray constants;
} Chunk;

void initChunk(Chunk *chunk);
void freeChunk(Chunk *chunk);
void writeChunk(Chunk *chunk, uint8_t byte, int line);
int addConstant(Chunk *chunk, Value value);
void writeConstant(Chunk *chunk, Value value, int line);
int getLine(Chunk *chunk, int code_index);

#endif
