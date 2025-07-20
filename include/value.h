#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum ValueType {
    VALUE_NONE,
    VALUE_INT,
    VALUE_FLOAT,
    VALUE_BOOL,
} ValueType;

typedef struct Value {
    ValueType type;
    union {
        int64_t int_;
        double float_;
        bool bool_;
    };
} Value;

#define INT_VALUE(value) ((Value) { .type = VALUE_INT, .int_ = value })
#define FLOAT_VALUE(value) ((Value) { .type = VALUE_FLOAT, .float_ = value })
#define BOOL_VALUE(value) ((Value) { .type = VALUE_BOOL, .bool_ = value })

const char* value_type_as_cstr(ValueType type);
