#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "environment.h"
#include "hash.h"
#include "strings.h"
#include "value.h"

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
        case VALUE_MODULE:   return "module";
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
        case VALUE_MODULE:   return strings_equal(a.module->name, b.module->name);
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
        case VALUE_MODULE: {
            printf("<module %s>", value.module->name->data);
        }
    }
}

String* string_create(int length, Hash hash, const char* data) {
    String* string = calloc(1, sizeof(String) + length + 1);
    string->length = length;
    string->hash = hash;
    memcpy(string->data, data, length);
    return string;
}

String* string_new(const char* data, int length) {
    return intern_string(data, length);
}

String* string_from(const char* data) {
    return intern_string(data, strlen(data));
}

String* string_concat(String* a, String* b) {
    int length = a->length + b->length;
    char* c = malloc(sizeof(char) * length);
    memcpy(c, a->data, a->length);
    memcpy(c + a->length, b->data, b->length);
    String* string = intern_string(c, length);
    free(c);
    return string;
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

Module* module_new(String* name, Environment* env) {
    Module* module = malloc(sizeof(Module));
    module->name = name;
    module->env = env;
    return module;
}
