#pragma once
#include <stdint.h>

struct String;

typedef uint32_t Hash;

Hash hash_cstring(const char* cstring, int length);
Hash hash_string(struct String* string);
