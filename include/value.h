#pragma once
#include <stdbool.h>

typedef enum ValueType {
    VALUE_NULL,
    VALUE_NUMBER,
    VALUE_BOOL,
    VALUE_STRING,
} ValueType;

typedef struct Value {
    ValueType type;
    union {
        double number;
        bool boolean;
        char* string;
    };
} Value;

#define IS_NULL(value)      (value.type == VALUE_NULL)
#define IS_NUMBER(value)    (value.type == VALUE_NUMBER)
#define IS_BOOL(value)      (value.type == VALUE_BOOL)
#define IS_STRING(value)    (value.type == VALUE_STRING)

#define NULL_VALUE()        ((Value){ .type = VALUE_NULL,   .number = 0 })
#define NUMBER_VALUE(value) ((Value){ .type = VALUE_NUMBER, .number = value })
#define BOOL_VALUE(value)   ((Value){ .type = VALUE_BOOL,   .boolean = value })
#define STRING_VALUE(value) ((Value){ .type = VALUE_STRING, .string = value })

void print_value(Value value);
