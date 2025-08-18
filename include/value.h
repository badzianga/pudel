#pragma once
#include <stdbool.h>

typedef enum {
    VALUE_NULL,
    VALUE_NUMBER,
    VALUE_BOOL,
    VALUE_STRING,
    VALUE_LIST,
    VALUE_NATIVE,
    VALUE_FUNCTION,
} ValueType;

typedef struct {
    int length;
    char data[];
} String;

typedef struct Value Value;

typedef struct {
    int length;
    int capacity;
    Value* values;
} List;

typedef Value (*NativeFn)(int argc, Value* argv);

struct ASTNode;

typedef struct {
    String* name;
    String** params;
    int param_count;
    struct ASTNode* body;
} Function;

struct Value {
    ValueType type;
    union {
        double number;
        bool boolean;
        String* string;
        List* list;
        NativeFn native;
        Function* function;
    };
};

#define IS_NULL(value)      ((value).type == VALUE_NULL)
#define IS_NUMBER(value)    ((value).type == VALUE_NUMBER)
#define IS_BOOL(value)      ((value).type == VALUE_BOOL)
#define IS_STRING(value)    ((value).type == VALUE_STRING)
#define IS_LIST(value)      ((value).type == VALUE_LIST)
#define IS_NATIVE(value)    ((value).type == VALUE_NATIVE)
#define IS_FUNCTION(value)  ((value).type == VALUE_FUNCTION)

#define NULL_VALUE()          ((Value){ .type = VALUE_NULL,     .number = 0 })
#define NUMBER_VALUE(value)   ((Value){ .type = VALUE_NUMBER,   .number = value })
#define BOOL_VALUE(value)     ((Value){ .type = VALUE_BOOL,     .boolean = value })
#define STRING_VALUE(value)   ((Value){ .type = VALUE_STRING,   .string = value })
#define LIST_VALUE(value)     ((Value){ .type = VALUE_LIST,     .list = value })
#define NATIVE_VALUE(value)   ((Value){ .type = VALUE_NATIVE,   .native = value })
#define FUNCTION_VALUE(value) ((Value){ .type = VALUE_FUNCTION, .function = value })

const char* value_type_as_cstr(ValueType type);

void print_value(Value value);
bool values_equal(Value a, Value b);

String* string_new(int length);
String* string_from(const char* data);
String* string_concat(String* a, String* b);
bool strings_equal(String* a, String* b);

List* list_new(int length);
bool lists_equal(List* a, List* b);
