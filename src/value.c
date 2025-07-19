#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "value.h"

const char* value_type_as_cstr(ValueType type) {
    static const char* value_type_strings[] = {
        "none",
        "int",
    };
    
    static_assert(
        sizeof(value_type_strings) / sizeof(value_type_strings[0]) == VALUE_INT + 1,
        "value::value_type_as_cstr: not all value types are handled"
    );

    if (type < 0 || type > VALUE_INT) {
        fprintf(stderr, "lexer::token_as_cstr: unknown token type with value: %d\n", type);
        exit(1);
    }

    return value_type_strings[type];
}
