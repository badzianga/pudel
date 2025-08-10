#include "environment.h"
#include "hashmap.h"

Environment env_new() {
    return (Environment){
        .map = hashmap_create()
    };
}

void env_free(Environment* env) {
    hashmap_free(&env->map);
}

bool env_define(Environment* env, String* name, Value value) {
    return hashmap_put(&env->map, name, value);
}

Value* env_get_ref(Environment* env, String* name) {
    return hashmap_get_ref(&env->map, name);
}
