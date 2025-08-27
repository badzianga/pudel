#pragma once
#include "hashmap.h"

void interned_strings_init();
void interned_strings_free();

String* intern_string(const char* data, int length);
