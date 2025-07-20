#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "value.h"

const char* value_type_as_cstr(ValueType type) {
    static const char* value_type_strings[] = {
        "none",
        "int",
        "float",
        "bool",
    };

    if (type < 0 || type > VALUE_BOOL) {
        fprintf(stderr, "lexer::token_as_cstr: unknown token type with value: %d\n", type);
        exit(1);
    }

    return value_type_strings[type];
}
