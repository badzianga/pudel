#include <stdio.h>
#include "value.h"

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
    }
}
