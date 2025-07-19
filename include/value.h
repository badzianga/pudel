#pragma once
#include <stdint.h>

typedef enum ValueType {
    VALUE_NONE,
    VALUE_INT,
} ValueType;

typedef struct Value {
    ValueType type;
    union {
        int64_t int_;
    };
} Value;

const char* value_type_as_cstr(ValueType type);
