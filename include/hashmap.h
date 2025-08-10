#pragma once
#include <stdint.h>
#include "value.h"

#define HASHMAP_INITIAL_CAPACITY 16
#define HASHMAP_LOAD_FACTOR 0.75

typedef struct {
    String* key;
    Value value;
} HashEntry;

typedef struct {
    HashEntry* entries;
    int capacity;
    int count;
} HashMap;

typedef uint32_t Hash;

HashMap hashmap_create();
void hashmap_free(HashMap* map);
bool hashmap_put(HashMap* map, String* key, Value value);
Value* hashmap_get_ref(HashMap* map, String* key);
