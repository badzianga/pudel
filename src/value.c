#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "value.h"
#include "hash.h"

const char* value_type_as_cstr(ValueType type) {
    switch (type) {
        case VALUE_NULL:     return "null";
        case VALUE_INT:      return "int";
        case VALUE_FLOAT:    return "float";
        case VALUE_BOOL:     return "bool";
        case VALUE_STRING:   return "string";
        case VALUE_LIST:     return "list";
        case VALUE_NATIVE:   return "native_func";
        case VALUE_FUNCTION: return "function";
    }
    return "unknown";
}

bool values_equal(Value a, Value b) {
    if (a.type != b.type)    return false;
    switch (a.type) {
        case VALUE_NULL:     return true;
        case VALUE_INT:      return a.integer == b.integer;
        case VALUE_BOOL:     return a.boolean == b.boolean;
        case VALUE_STRING:   return strings_equal(a.string, b.string);
        case VALUE_LIST:     return lists_equal(a.list, b.list);
        case VALUE_NATIVE:   return a.native == b.native;
        case VALUE_FUNCTION: return strings_equal(a.function->name, b.function->name);
        default:             return false;
    }
}

void print_value(Value value) {
    switch (value.type) {
        case VALUE_NULL: {
            fputs("null", stdout);
        } break;
        case VALUE_INT: {
            printf("%ld", value.integer);
        } break;
        case VALUE_FLOAT: {
            printf("%g", value.floating);
        } break;
        case VALUE_BOOL: {
            printf("%s", value.boolean ? "true" : "false");
        } break;
        case VALUE_STRING: {
            printf("%s", value.string->data);
        } break;
        case VALUE_LIST: {
            fputs("[", stdout);
            for (int i = 0; i < value.list->length; ++i) {
                if (value.list->values[i].type != VALUE_STRING) {
                    print_value(value.list->values[i]);
                }
                else {
                    putchar('"');
                    print_value(value.list->values[i]);
                    putchar('"');
                }
                if (i < value.list->length - 1) {
                    printf(", ");
                }
            }
            fputs("]", stdout);
        } break;
        case VALUE_NATIVE: {
            printf("<native>");  // TODO: print proper native function value
        } break;
        case VALUE_FUNCTION: {
            printf("<function %s>", value.function->name->data);
        } break;
    }
}

String* string_new(const char* data, int length) {
    String* string = calloc(1, sizeof(String) + length + 1);
    string->length = length;
    string->hash = hash_cstring(data, length);
    memcpy(string->data, data, length);
    return string;
}

// FIXME: most of the strings created using this function are not freed - especially when defining variables 
// because of that there are huuuuuge memory leaks in recursive functions
// one of the solutions should be string interning
String* string_from(const char* data) {
    String* string = string_new(data, strlen(data));
    return string;
}

// FIXME: strings created using this method aren't freed (only string literals)
String* string_concat(String* a, String* b) {
    int length = a->length + b->length;
    String* c = calloc(1, sizeof(String) + length + 1);
    c->length = length;
    memcpy(c->data, a->data, a->length);
    memcpy(c->data + a->length, b->data, b->length);
    c->hash = hash_string(c);
    return c;
}

bool strings_equal(String* a, String* b) {
    return a->hash == b->hash && a->length == b->length && strcmp(a->data, b->data) == 0;
}

List* list_new(int length) {
    List* list = calloc(1, sizeof(List));
    list->values = calloc(length, sizeof(Value));
    list->capacity = length;
    return list;
}

bool lists_equal(List* a, List* b) {
    if (a->length != b->length) return false;
    for (int i = 0; i < a->length; ++i) {
        if (!values_equal(a->values[i], b->values[i])) return false;
    }
    return true;
}
