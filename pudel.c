#include <stdio.h>
#include <stdlib.h>
#include "debug.h"
#include "interpreter.h"
#include "io.h"
#include "lexer.h"
#include "parser.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <input.pud>\n", argv[0]);
        exit(1);
    }
    char* source = file_read(argv[1]);

    TokenArray token_array = lexer_lex(source);
    debug_print_token_array(&token_array);

    printf("----------------------------------------------------------------\n");

    ASTNode* ast = parser_parse(&token_array);
    debug_print_ast(ast, 0);

    printf("----------------------------------------------------------------\n");

    interpreter_interpret(ast);

    parser_free_ast(ast);
    lexer_free_tokens(&token_array);
    free(source);
    return 0;
}
