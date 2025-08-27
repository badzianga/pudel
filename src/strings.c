#include <stdlib.h>
#include <string.h>
#include "strings.h"
#include "hash.h"
#include "hashmap.h"
#include "value.h"

typedef struct {
    String** entries;
    int count;
    int capacity;
} StringTable;

static StringTable strings = { 0 };

static void resize(int new_capacity) {
    String** new_entries = calloc(new_capacity, sizeof(String*));

    for (int i = 0; i < strings.capacity; ++i) {
        String* entry = strings.entries[i];
        if (entry != NULL) {
            int index = entry->hash % new_capacity;

            while (new_entries[index] != NULL) {
                index = (index + 1) % new_capacity;
            }

            new_entries[index] = entry;
        }
    }

    free(strings.entries);
    strings.entries = new_entries;
    strings.capacity = new_capacity;
}

void interned_strings_init() {
    strings.entries = calloc(HASHMAP_INITIAL_CAPACITY, sizeof(StringTable));
    strings.capacity = HASHMAP_INITIAL_CAPACITY;
    strings.count = 0;
}

void interned_strings_free() {
    for (int i = 0; i < strings.capacity; ++i) {
        if (strings.entries[i] != NULL) {
            free(strings.entries[i]);
        }
    }
    free(strings.entries);
    
    strings.entries = NULL;
    strings.capacity = 0;
    strings.count = 0;
}

String* intern_string(const char* data, int length) {
    if (strings.count + 1 > strings.capacity * HASHMAP_LOAD_FACTOR) {
        resize(strings.capacity * 2);
    }

    Hash hash = hash_cstring(data, length);

    int index = hash % strings.capacity;
    String* entry = NULL;
    while ((entry = strings.entries[index]) != NULL) {
        if (entry->length == length && memcmp(entry->data, data, length) == 0) {
            // printf("[DEBUG] Found interned string. Current count: %d. String: \"%s\"\n", strings.count, entry->data);
            return entry;  // found interned string
        }
        index = (index + 1) % strings.capacity;
    }

    // not found - create new string
    String* string = string_create(length, hash, data);
    strings.entries[index] = string;
    ++strings.count;

    // printf("[DEBUG] Interned new string.   Current count: %d. String: \"%s\"\n", strings.count, string->data);
    return string;
}
