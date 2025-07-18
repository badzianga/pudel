#pragma once
#include "lexer.h"
#include "parser.h"

void debug_print_token_array(TokenArray* token_array);
void debug_print_ast(ASTNode* root, int indent);
