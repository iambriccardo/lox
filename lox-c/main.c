#include "chunk.h"
#include "vm.h"

int main(int argc, const char *argv[]) {
  initVM();

  Chunk chunk;
  initChunk(&chunk);

  writeConstant(&chunk, 3.4, 123);
  writeConstant(&chunk, 1.4, 123);
  writeChunk(&chunk, OP_ADD, 123);
  writeChunk(&chunk, OP_NEGATE, 123);
  writeChunk(&chunk, OP_RETURN, 123);
  // writeConstant(&chunk, 2.0, 123);
  // writeChunk(&chunk, OP_DIVIDE, 123);

  interpret(&chunk);

  freeVM();
  freeChunk(&chunk);
}
