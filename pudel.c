#include <stdlib.h>
#include "debug.h"
#include "io.h"
#include "lexer.h"

int main() {
    char* source = file_read("examples/expression.pud");

    TokenArray token_array = lexer_lex(source);
    debug_print_token_array(&token_array);

    lexer_free_tokens(&token_array);
    free(source);
    return 0;
}
