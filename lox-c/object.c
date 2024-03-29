#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objectType)                                         \
  (type *)allocateObject(sizeof(type), objectType)

static Obj *allocateObject(size_t size, ObjType type) {
  Obj *object = (Obj *)reallocate(NULL, 0, size);
  object->type = type;

  object->next = vm.objects;
  vm.objects = object;

  return object;
}

static ObjString *allocateString(char chars[], int length, uint32_t hash) {
  ObjString *string = (ObjString *)allocateObject(
      sizeof(ObjString) + sizeof(char[length]), OBJ_STRING);
  string->start = NULL;
  string->hash = hash;
  string->length = length;
  strcpy(string->chars, chars);

  tableSet(&vm.strings, OBJ_VAL(string), NIL_VAL);

  return string;
}

static ObjString *allocateReferenceString(const char *start, int length,
                                          uint32_t hash) {
  ObjString *string =
      (ObjString *)allocateObject(sizeof(ObjString), OBJ_STRING);
  string->start = start;
  string->hash = hash;
  string->length = length;

  tableSet(&vm.strings, OBJ_VAL(string), NIL_VAL);

  return string;
}

static uint32_t hashString(const char *key, int length) {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

ObjString *takeString(char chars[], int length) {
  uint32_t hash = hashString(chars, length);
  ObjString *interned = tableFindString(&vm.strings, chars, length, hash);
  if (interned != NULL)
    return interned;

  return allocateString(chars, length, hash);
}

ObjString *copyString(const char chars[], int length) {
  uint32_t hash = hashString(chars, length);
  ObjString *interned = tableFindString(&vm.strings, chars, length, hash);
  if (interned != NULL)
    return interned;

  char *heapChars = ALLOCATE(char, length + 1);
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';
  return allocateString(heapChars, length, hash);
}

ObjString *referenceString(const char *start, int length) {
  uint32_t hash = hashString(start, length);
  ObjString *interned = tableFindString(&vm.strings, start, length, hash);
  if (interned != NULL)
    return interned;

  return allocateReferenceString(start, length, hash);
}

void printObject(Value value) {
  switch (OBJ_TYPE(value)) {
  case OBJ_STRING: {
    ObjString *objString = AS_STRING(value);
    if (IS_REF_STRING(objString)) {
      printf("ref:");
      for (int i = 0; i < objString->length; i++) {
        printf("%c", objString->start[i]);
      }
    } else {
      printf("copy:");
      printf("%s", AS_CSTRING(value));
    }
    break;
  }
  }
}
