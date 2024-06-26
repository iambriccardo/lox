#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "table.h"
#include "value.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
  ObjClosure *closure;
  uint8_t *ip;
  Value *slots;
} CallFrame;

typedef struct {
  CallFrame frames[FRAMES_MAX];
  int frameCount;

  Chunk *chunk;
  uint8_t *ip;
  Value stack[STACK_MAX];
  Value *stackTop;
  // Table of global variables.
  Table globals;
  // Table of the interned strings.
  Table strings;
  ObjUpvalue *openUpvalues;

  unsigned int gcState : 1;
  size_t bytesAllocated;
  size_t nextGC;
  Obj *objects;
  int grayCount;
  int grayCapacity;
  Obj **grayStack;
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

void initVM();
void freeVM();
InterpretResult interpret(const char *chunk);
void push(Value value);
Value pop();

#endif
