#include <stdio.h>
#include <stdlib.h>
#include "debug.h"
#include "io.h"
#include "lexer.h"
#include "parser.h"

int main() {
    char* source = file_read("examples/expression.pud");

    TokenArray token_array = lexer_lex(source);
    debug_print_token_array(&token_array);

    printf("----------------------------------------------------------------\n");

    ASTNode* ast = parser_parse(&token_array);
    debug_print_ast(ast, 0);

    parser_free_ast(ast);
    lexer_free_tokens(&token_array);
    free(source);
    return 0;
}
