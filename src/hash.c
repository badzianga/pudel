#include "hash.h"
#include "value.h"

Hash hash_cstring(const char* cstring, int length) {
    Hash hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)cstring[i];
        hash *= 16777619;
    }
    return hash;
}

Hash hash_string(String* string) {
    return hash_cstring(string->data, string->length);
}
