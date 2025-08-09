#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "value.h"

bool values_equal(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case VALUE_NULL:   return true;
        case VALUE_NUMBER: return a.number == b.number;
        case VALUE_BOOL:   return a.boolean == b.boolean;
        case VALUE_STRING: return strings_equal(a.string, b.string);
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
    }
}

String* string_new(int length) {
    String* string = calloc(1, sizeof(String) + length + 1);
    string->length = length;
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
