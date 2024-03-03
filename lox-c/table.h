#ifndef clox_table_h
#define clox_table_h

#include "common.h"
#include "object.h"
#include "value.h"

// TODO: to add multiple data types as key we must:
// 1. Use `Value` as key
// 2. Build a function to compute the hash of a value
// 3. Build a function to compare two values
typedef struct {
  Value key;
  Value value;
} Entry;

typedef struct {
  int count;
  int capacity;
  Entry *entries;
} Table;

void initTable(Table *table);
void freeTable(Table *table);
bool tableSet(Table *table, Value key, Value value);
bool tableDelete(Table *table, Value key);
bool tableGet(Table *table, Value key, Value *value);
void tableAddAll(Table *from, Table *to);
ObjString *tableFindString(Table *table, const char *chars, int length,
                           uint32_t hash);

#endif
