#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void initTable(Table *table) {
  table->count = 0;
  table->capacity = 0;
  table->entries = NULL;
}

void freeTable(Table *table) {
  FREE_ARRAY(Entry, table->entries, table->capacity);
  initTable(table);
}

static Entry *findEntry(Entry *entries, int capacity, Value key) {
  uint32_t index = valueHash(key) % capacity;
  Entry *tombstone = NULL;

  for (;;) {
    Entry *entry = &entries[index];
    if (IS_NIL(entry->key)) {
      if (IS_NIL(entry->value)) {
        // Empty entry.
        return tombstone != NULL ? tombstone : entry;
      } else {
        // We found a tombstone.
        if (tombstone == NULL) {
          tombstone = entry;
        }
      }
    } else if (valuesEqual(entry->key, key)) {
      // We found the key.
      return entry;
    }

    index = (index + 1) % capacity;
  }
}

static void adjustCapacity(Table *table, int capacity) {
  Entry *entries = ALLOCATE(Entry, capacity);
  for (int i = 0; i < capacity; i++) {
    entries[i].key = NIL_VAL;
    entries[i].value = NIL_VAL;
  }

  table->count = 0;
  for (int i = 0; i < table->capacity; i++) {
    Entry *entry = &table->entries[i];
    if (IS_NIL(entry->key)) {
      continue;
    }

    Entry *dest = findEntry(entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    table->count++;
  }

  FREE_ARRAY(Entry, table->entries, table->capacity);
  table->entries = entries;
  table->capacity = capacity;
}

bool tableSet(Table *table, Value key, Value value) {
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(table->capacity);
    adjustCapacity(table, capacity);
  }

  Entry *entry = findEntry(table->entries, table->capacity, key);
  bool isNewKey = IS_NIL(entry->key);
  if (isNewKey && IS_NIL(entry->value)) {
    table->count++;
  }

  entry->key = key;
  entry->value = value;
  return isNewKey;
}

bool tableDelete(Table *table, Value key) {
  if (table->count == 0)
    return false;

  // Find the entry.
  Entry *entry = findEntry(table->entries, table->capacity, key);
  if (IS_NIL(entry->key)) {
    return false;
  }

  // Place a tombstone in the entry.
  Value nilValue;
  nilValue.type = VAL_NIL;
  entry->key = nilValue;
  entry->value = BOOL_VAL(true);
  return true;
}

bool tableGet(Table *table, Value key, Value *value) {
  if (table->count == 0)
    return false;

  Entry *entry = findEntry(table->entries, table->capacity, key);
  if (IS_NIL(entry->key)) {
    return false;
  }

  *value = entry->value;
  return true;
}

void tableAddAll(Table *from, Table *to) {
  for (int i = 0; i < from->capacity; i++) {
    Entry *entry = &from->entries[i];
    if (!IS_NIL(entry->key)) {
      tableSet(to, entry->key, entry->value);
    }
  }
}

ObjString *tableFindString(Table *table, const char *chars, int length,
                           uint32_t hash) {
  if (table->count == 0) {
    return NULL;
  }

  uint32_t index = hash % table->capacity;
  for (;;) {
    Entry *entry = &table->entries[index];
    if (IS_NIL(entry->key)) {
      // Stop if we find an empty non-tombstone entry.
      if (IS_NIL(entry->value)) {
        return NULL;
      }
    } else if (IS_STRING(entry->key)) {
      ObjString *objString = AS_STRING(entry->key);
      if (objString->length == length && valueHash(entry->key) == hash &&
          memcmp(STRING_CHARS(objString), chars, length) == 0) {
        return objString;
      }
    }

    index = (index + 1) % table->capacity;
  }
}

void tableRemoveWhite(Table *table) {
  for (int i = 0; i < table->capacity; i++) {
    Entry *entry = &table->entries[i];
    Obj *object = AS_OBJ(entry->key);
    if (!IS_NIL(entry->key) && object != NULL && !object->isMarked) {
      printValue(entry->key);
      tableDelete(table, entry->key);
    }
  }
}

void markTable(Table *table) {
  for (int i = 0; i < table->capacity; i++) {
    Entry *entry = &table->entries[i];
    markValue(entry->key);
    markValue(entry->value);
  }
}
