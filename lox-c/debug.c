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

int disassembleInstruction(Chunk *chunk, int offset) {
  printf("%04d ", offset);

  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
  case OP_RETURN:
    return simpleInstruction("OP_RETURN", offset);
  case OP_CONSTANT:
    return constantInstruction("OP_CONSTANT", chunk, offset);
  default:
    printf("Unknown opcode %d\n", instruction);
    return offset + 1;
  }
}
