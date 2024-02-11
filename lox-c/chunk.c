#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

void initChunk(Chunk *chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  chunk->line_ranges_count = 0;
  chunk->line_ranges = NULL;
  initValueArray(&chunk->constants);
}

void freeChunk(Chunk *chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  FREE_ARRAY(int, chunk->line_ranges, chunk->capacity);
  freeValueArray(&chunk->constants);
  initChunk(chunk);
}

void writeChunk(Chunk *chunk, uint8_t byte, int line) {
  if (chunk->capacity < chunk->count + 1) {
    int oldCapacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(oldCapacity);
    chunk->code =
        GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
    // We grow the line ranges at the same rante as the code, assuming that in
    // the worst case we have a different line per code entry.
    chunk->line_ranges =
        GROW_ARRAY(LineRange, chunk->line_ranges, oldCapacity, chunk->capacity);
  }

  if (chunk->line_ranges_count == 0 ||
      chunk->line_ranges[chunk->line_ranges_count - 1].lineno != line) {
    LineRange line_range;
    line_range.code_start = chunk->count;
    line_range.code_end = chunk->count;
    line_range.lineno = line;
    chunk->line_ranges[chunk->line_ranges_count] = line_range;
    chunk->line_ranges_count++;
  } else if (chunk->line_ranges[chunk->line_ranges_count - 1].lineno == line) {
    chunk->line_ranges[chunk->line_ranges_count - 1].code_end = chunk->count;
  }

  chunk->code[chunk->count] = byte;
  chunk->count++;
}

void writeConstant(Chunk *chunk, Value value, int line) {
  writeChunk(chunk, OP_CONSTANT_LONG, line);

  int constant_index = addConstant(chunk, value);
  uint8_t bytes[] = {constant_index & 0xff, (constant_index >> 8) & 0xff,
                     (constant_index >> 16 & 0xff)};
  for (int i = 0; i < 3; i++) {
    writeChunk(chunk, bytes[i], line);
  }
}

int addConstant(Chunk *chunk, Value value) {
  // TODO: check how to inject the line information of the constant.
  writeValueArray(&chunk->constants, value);
  return chunk->constants.count - 1;
}

int getLine(Chunk *chunk, int code_index) {
  for (int i = 0; i < chunk->line_ranges_count; i++) {
    LineRange line_range = chunk->line_ranges[i];
    if (code_index >= line_range.code_start &&
        code_index <= line_range.code_end) {
      return line_range.lineno;
    }
  }

  return -1;
}
