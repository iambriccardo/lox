#ifndef clox_object_h
#define clox_object_h

#include "chunk.h"
#include "common.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_CLOSURE(value) isObjType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)
#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_CLOSURE(value) ((ObjClosure *)AS_OBJ(value))
#define AS_FUNCTION(value) ((ObjFunction *)AS_OBJ(value))
#define AS_NATIVE(value) (((ObjNative *)AS_OBJ(value))->function)
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)

#define STRING_CHARS(objString)                                                \
  objString->start != NULL ? objString->start : objString->chars
#define IS_REF_STRING(objString) objString->start != NULL

typedef enum {
  OBJ_CLOSURE,
  OBJ_FUNCTION,
  OBJ_NATIVE,
  OBJ_STRING,
} ObjType;

struct Obj {
  ObjType type;
  // Field used to keep track of all allocated objects.
  struct Obj *next;
};

struct ObjFunction {
  Obj obj;
  int arity;
  int upvalueCount;
  Chunk chunk;
  ObjString *name;
};

typedef Value (*NativeFn)(int argCount, Value *args);

struct ObjNative {
  Obj obj;
  NativeFn function;
};

struct ObjString {
  Obj obj;
  const char *start;
  uint32_t hash;
  int length;
  char chars[];
};

struct ObjClosure {
  Obj obj;
  ObjFunction *function;
};

ObjClosure *newClosure(ObjFunction *function);
ObjFunction *newFunction();
ObjNative *newNative(NativeFn function);
ObjString *takeString(char *chars, int length);
ObjString *copyString(const char *chars, int length);
ObjString *referenceString(const char *start, int length);
void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
