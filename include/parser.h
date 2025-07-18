#pragma once
#include "lexer.h"

typedef enum ASTNodeType {
    AST_NODE_PROGRAM,
    AST_NODE_EXPRESSION_STATEMENT,
    AST_NODE_PRINT_STATEMENT,

    AST_NODE_BINARY,
    AST_NODE_UNARY,
    AST_NODE_LITERAL,
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;

    union {
        struct {
            struct ASTNode** statements;
            int count;
            int capacity;
        } scope;

        struct ASTNode* expression;

        struct {
            struct ASTNode* left;
            TokenType op;
            struct ASTNode* right;
        } binary;

        struct {
            TokenType op;
            struct ASTNode* right;
        } unary;

        int literal;
    };
} ASTNode;

ASTNode* parser_parse(TokenArray* token_array);
void parser_free_ast(ASTNode* root);
