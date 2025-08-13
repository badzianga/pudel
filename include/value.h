#pragma once
#include <stdbool.h>

typedef enum {
    VALUE_NULL,
    VALUE_NUMBER,
    VALUE_BOOL,
    VALUE_STRING,
    VALUE_NATIVE,
} ValueType;

typedef struct {
    int length;
    char data[];
} String;

typedef struct Value Value;

typedef Value (*NativeFn)(int argc, Value* argv);

struct Value {
    ValueType type;
    union {
        double number;
        bool boolean;
        String* string;
        NativeFn native;
    };
};

#define IS_NULL(value)      ((value).type == VALUE_NULL)
#define IS_NUMBER(value)    ((value).type == VALUE_NUMBER)
#define IS_BOOL(value)      ((value).type == VALUE_BOOL)
#define IS_STRING(value)    ((value).type == VALUE_STRING)
#define IS_NATIVE(value)    ((value).type == VALUE_NATIVE)

#define NULL_VALUE()          ((Value){ .type = VALUE_NULL,   .number = 0 })
#define NUMBER_VALUE(value)   ((Value){ .type = VALUE_NUMBER, .number = value })
#define BOOL_VALUE(value)     ((Value){ .type = VALUE_BOOL,   .boolean = value })
#define STRING_VALUE(value)   ((Value){ .type = VALUE_STRING, .string = value })
#define NATIVE_VALUE(value)   ((Value){ .type = VALUE_NATIVE, .native = value })

void print_value(Value value);
bool values_equal(Value a, Value b);

String* string_new(int length);
String* string_concat(String* a, String* b);
bool strings_equal(String* a, String* b);
