#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
  OP_CONSTANT,
  OP_CONSTANT_LONG,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NEGATE,
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
