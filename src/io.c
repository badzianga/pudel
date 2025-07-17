#include <stdio.h>
#include <stdlib.h>
#include "io.h"

char* file_read(const char* file_path) {
    FILE* file = fopen(file_path, "rb");
    if (file == NULL) {
        fprintf(stderr, "io::file_read: failed to open file: %s\n", file_path);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    int length = ftell(file);
    rewind(file);

    char* content = (char*)malloc(sizeof(char) * length + 1);
    if (content == NULL) {
        fprintf(stderr, "io::file_read: failed to allocate memory for file: %s\n", file_path);
        exit(1);
    }

    fread(content, 1, length, file);
    content[length] = '\0';

    fclose(file);

    return content;
}
