#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum ValueType {
    VALUE_NULL,
    VALUE_NUMBER,
    VALUE_BOOL,
} ValueType;

typedef struct Value {
    ValueType type;
    union {
        double number;
        bool boolean;
    };
} Value;

#define NULL_VALUE()        ((Value){ .type = VALUE_NULL,   .number = 0 })
#define NUMBER_VALUE(value) ((Value){ .type = VALUE_NUMBER, .number = value })
#define BOOL_VALUE(value)   ((Value){ .type = VALUE_BOOL,   .boolean = value })
