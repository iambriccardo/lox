#include <stdio.h>

#include "chunk.h"
#include "debug.h"

void disassembleChunk(Chunk *chunk, const char *name) {
  printf("== %s ==\n", name);

  for (int offset = 0; offset < chunk->count;) {
    offset = disassembleInstruction(chunk, offset);
  }
}

static int simpleInstruction(const char *name, int offset) {
  // A simple instruction is stored in one byte containing the op code.
  printf("%s\n", name);
  return offset + 1;
}

static int constantInstruction(const char *name, Chunk *chunk, int offset) {
  // A constant instruction is stored in two bytes, one for op code and one for
  // operand.
  uint8_t constant_index = chunk->code[offset + 1];
  printf("%-16s %4d '", name, constant_index);
  printValue(chunk->constants.values[constant_index]);
  printf("'\n");
  return offset + 2;
}

static int longConstantInstruction(const char *name, Chunk *chunk, int offset) {
  // A constant instruction is stored in 4 bytes, one for op code and three for
  // the 24 bit operand.
  int constant_index = ((int)chunk->code[offset + 3]) << 16;
  constant_index |= ((int)chunk->code[offset + 2]) << 8;
  constant_index |= chunk->code[offset + 1];

  printf("%-16s %4d '", name, constant_index);
  printValue(chunk->constants.values[constant_index]);
  printf("'\n");
  return offset + 4;
}

int disassembleInstruction(Chunk *chunk, int offset) {
  printf("%04d ", offset);

  int previous_line = getLine(chunk, offset - 1);
  int line = getLine(chunk, offset);
  if (offset > 0 && line == previous_line) {
    printf("	| ");
  } else {
    printf("%4d ", line);
  }

  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
  case OP_CONSTANT:
    return constantInstruction("OP_CONSTANT", chunk, offset);
  case OP_CONSTANT_LONG:
    return longConstantInstruction("OP_CONSTANT_LONG", chunk, offset);
  case OP_ADD:
    return simpleInstruction("OP_ADD", offset);
  case OP_SUBTRACT:
    return simpleInstruction("OP_SUBTRACT", offset);
  case OP_MULTIPLY:
    return simpleInstruction("OP_MULTIPLY", offset);
  case OP_DIVIDE:
    return simpleInstruction("OP_DIVIDE", offset);
  case OP_NEGATE:
    return simpleInstruction("OP_NEGATE", offset);
  case OP_RETURN:
    return simpleInstruction("OP_RETURN", offset);
  default:
    printf("Unknown opcode %d\n", instruction);
    return offset + 1;
  }
}
