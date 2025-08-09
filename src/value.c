#include <stdio.h>
#include <string.h>
#include "value.h"

bool values_equal(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case VALUE_NULL:   return true;
        case VALUE_NUMBER: return a.number == b.number;
        case VALUE_BOOL:   return a.boolean == b.boolean;
        case VALUE_STRING: return strcmp(a.string, b.string) == 0;
        default:           return false;
    }
}

void print_value(Value value) {
    switch (value.type) {
        case VALUE_NULL: {
            fputs("null", stdout);
        } break;
        case VALUE_NUMBER: {
            printf("%lf", value.number);
        } break;
        case VALUE_BOOL: {
            printf("%s", value.boolean ? "true" : "false");
        } break;
        case VALUE_STRING: {
            printf("%s", value.string);
        } break;
    }
}
