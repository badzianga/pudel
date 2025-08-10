#pragma once
#include "hashmap.h"

typedef struct Environment {
    HashMap map;
} Environment;

Environment env_new();
void env_free(Environment* env);

bool env_define(Environment* env, String* name, Value value);
Value* env_get_ref(Environment* env, String* name);
