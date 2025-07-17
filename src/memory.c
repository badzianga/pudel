#include <stdio.h>
#include <stdlib.h>
#include "memory.h"

void* reallocate(void* pointer, int new_size) {
    if (new_size == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, new_size);
    if (result == NULL) {
        fprintf(stderr, "memory::reallocate: cannot allocate enough memory\n");
        exit(1);
    }
    return result;
}
