#pragma once
#include "hashmap.h"

typedef struct Environment {
    struct Environment* enclosing;
    HashMap map;
} Environment;

Environment* env_new();
Environment* env_new_with_enclosing(Environment* env);
void env_free(Environment* env);

bool env_define(Environment* env, String* name, Value value);
Value* env_get_ref(Environment* env, String* name);
