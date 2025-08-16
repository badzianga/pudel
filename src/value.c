#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "value.h"


const char* value_type_as_cstr(ValueType type) {
    switch (type) {
        case VALUE_NULL:   return "null";
        case VALUE_NUMBER: return "number";
        case VALUE_BOOL:   return "bool";
        case VALUE_STRING: return "string";
        case VALUE_LIST:   return "list";
        case VALUE_NATIVE: return "native_func";
    }
    return "unknown";
}

bool values_equal(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case VALUE_NULL:   return true;
        case VALUE_NUMBER: return a.number == b.number;
        case VALUE_BOOL:   return a.boolean == b.boolean;
        case VALUE_STRING: return strings_equal(a.string, b.string);
        case VALUE_LIST:   return lists_equal(a.list, b.list);
        case VALUE_NATIVE: return a.native == b.native;
        default:           return false;
    }
}

void print_value(Value value) {
    switch (value.type) {
        case VALUE_NULL: {
            fputs("null", stdout);
        } break;
        case VALUE_NUMBER: {
            printf("%g", value.number);
        } break;
        case VALUE_BOOL: {
            printf("%s", value.boolean ? "true" : "false");
        } break;
        case VALUE_STRING: {
            printf("%s", value.string->data);
        } break;
        case VALUE_LIST: {
            printf("<list>");  // TODO: print list content with raw strings = ["hello\t"] instead of [hello  ] 
        } break;
        case VALUE_NATIVE: {
            printf("<native>");  // TODO: print proper native function value
        }
    }
}

String* string_new(int length) {
    String* string = calloc(1, sizeof(String) + length + 1);
    string->length = length;
    return string;
}

String* string_from(const char* data) {
    String* string = string_new(strlen(data));
    memcpy(string->data, data, string->length);
    return string;
}

// TODO: strings created using this method aren't freed (only string literals)
String* string_concat(String* a, String* b) {
    String* c = string_new(a->length + b->length);
    memcpy(c->data, a->data, a->length);
    memcpy(c->data + a->length, b->data, b->length);
    return c;
}

bool strings_equal(String* a, String* b) {
    return a->length == b->length && strcmp(a->data, b->data) == 0;
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
