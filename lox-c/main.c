#include "chunk.h"
#include "vm.h"

int main(int argc, const char *argv[]) {
  initVM();
  Chunk chunk;
  initChunk(&chunk);
  interpret(&chunk);
  freeVM();
  freeChunk(&chunk);
}
