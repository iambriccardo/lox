#ifndef clox_object_h
#define clox_object_h

#include "chunk.h"
#include "common.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_FUNCTION(value) ((ObjFunction *)AS_OBJ(value))
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)

#define STRING_CHARS(objString)                                                \
  objString->start != NULL ? objString->start : objString->chars
#define IS_REF_STRING(objString) objString->start != NULL

typedef enum {
  OBJ_FUNCTION,
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
  Chunk chunk;
  ObjString *name;
};

struct ObjString {
  Obj obj;
  const char *start;
  uint32_t hash;
  int length;
  char chars[];
};

ObjFunction *newFunction();
ObjString *takeString(char *chars, int length);
ObjString *copyString(const char *chars, int length);
ObjString *referenceString(const char *start, int length);
void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
