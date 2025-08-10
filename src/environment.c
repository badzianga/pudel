#include <stdlib.h>
#include "environment.h"
#include "hashmap.h"

Environment* env_new() {
    Environment* new_env = malloc(sizeof(Environment));
    new_env->enclosing = NULL;
    new_env->map = hashmap_create();
    return new_env;
}

Environment* env_new_with_enclosing(Environment* env) {
    Environment* new_env = malloc(sizeof(Environment));
    new_env->enclosing = env;
    new_env->map = hashmap_create();
    return new_env;
}

void env_free(Environment* env) {
    hashmap_free(&env->map);
    free(env);
}

bool env_define(Environment* env, String* name, Value value) {
    return hashmap_put(&env->map, name, value);
}

Value* env_get_ref(Environment* env, String* name) {
    Value* ref = hashmap_get_ref(&env->map, name);
    if (ref != NULL) {
        return ref;
    }

    if (env->enclosing != NULL) {
        return env_get_ref(env->enclosing, name);
    }

    return NULL;
}
