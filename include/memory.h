#pragma once

#define GROW_CAPACITY(capacity) \
    ((capacity) < 4 ? 4 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, new_count) \
    (type*)reallocate(pointer, sizeof(type) * (new_count))

void* reallocate(void* pointer, int new_size);
