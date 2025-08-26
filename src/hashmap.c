#include <stdlib.h>
#include <string.h>
#include "hashmap.h"

static void hashmap_resize(HashMap* map, int new_capacity) {
    HashEntry* new_entries = calloc(new_capacity, sizeof(HashEntry));

    for (int i = 0; i < map->capacity; ++i) {
        HashEntry entry = map->entries[i];
        if (entry.key != NULL) {
            int index = entry.key->hash % new_capacity;

            while (new_entries[index].key != NULL) {
                index = (index + 1) % new_capacity;
            }

            new_entries[index] = entry;
        }
    }

    free(map->entries);
    map->entries = new_entries;
    map->capacity = new_capacity;
}

HashMap hashmap_create() {
    return (HashMap){
        .entries = calloc(HASHMAP_INITIAL_CAPACITY, sizeof(HashEntry)),
        .capacity = HASHMAP_INITIAL_CAPACITY,
        .count = 0
    };
}

void hashmap_free(HashMap* map) {
    free(map->entries);
    map->entries = NULL;
    map->capacity = 0;
    map->count = 0;
}

bool hashmap_put(HashMap* map, String* key, Value value) {
    if (map->count + 1 > map->capacity * HASHMAP_LOAD_FACTOR) {
        hashmap_resize(map, map->capacity * 2);
    }

    int index = key->hash % map->capacity;

    while (map->entries[index].key != NULL) {
        if (map->entries[index].key->length == key->length && strcmp(map->entries[index].key->data, key->data) == 0) {
            // key exists, update value
            map->entries[index].value = value;
            return true;
        }
        index = (index + 1) % map->capacity;
    }

    // insert new key-value pair
    map->entries[index].key = key;
    map->entries[index].value = value;
    ++map->count;
    return false;
}

Value* hashmap_get_ref(HashMap* map, String* key) {
    int index = key->hash % map->capacity;

    while (map->entries[index].key != NULL) {
        if (map->entries[index].key->length == key->length && strcmp(map->entries[index].key->data, key->data) == 0) {
            return &map->entries[index].value;
        }
        index = (index + 1) % map->capacity;
    }

    return NULL;
}
