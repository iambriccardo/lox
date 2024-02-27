#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
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

static ObjString *allocateString(char chars[], int length) {
  ObjString *string = (ObjString *)allocateObject(
      sizeof(ObjString) + sizeof(char[length]), OBJ_STRING);
  string->start = NULL;
  string->length = length;
  strcpy(string->chars, chars);
  return string;
}

static ObjString *allocateReferenceString(const char *start, int length) {
  ObjString *string =
      (ObjString *)allocateObject(sizeof(ObjString), OBJ_STRING);
  string->start = start;
  string->length = length;
  return string;
}

ObjString *takeString(char chars[], int length) {
  return allocateString(chars, length);
}

ObjString *copyString(const char chars[], int length) {
  char *heapChars = ALLOCATE(char, length + 1);
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';
  return allocateString(heapChars, length);
}

ObjString *referenceString(const char *start, int length) {
  return allocateReferenceString(start, length);
}

void printObject(Value value) {
  switch (OBJ_TYPE(value)) {
  case OBJ_STRING: {
    ObjString *objString = AS_STRING(value);
    if (objString->start != NULL) {
      printf("referenced:");
      for (int i = 0; i < objString->length; i++) {
        printf("%c", objString->start[i]);
      }
    } else {
      printf("copied:");
      printf("%s", AS_CSTRING(value));
    }
    break;
  }
  }
}
