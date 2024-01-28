#include "chunk.h"
#include "debug.h"

int main(int argc, const char *argv[]) {
  Chunk chunk;
  initChunk(&chunk);

  writeConstant(&chunk, 150, 121);
  writeChunk(&chunk, OP_RETURN, 123);

  disassembleChunk(&chunk, "test chunk");
  freeChunk(&chunk);
}
