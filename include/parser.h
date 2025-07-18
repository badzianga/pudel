#pragma once
#include "lexer.h"

typedef enum ASTNodeType {
    AST_NODE_PROGRAM,
    AST_NODE_EXPRESSION_STATEMENT,
    AST_NODE_PRINT_STATEMENT,
    AST_NODE_IF_STATEMENT,
    AST_NODE_WHILE_STATEMENT,
    AST_NODE_BLOCK,

    AST_NODE_LOGICAL,
    AST_NODE_BINARY,
    AST_NODE_UNARY,
    AST_NODE_LITERAL,
    AST_NODE_VARIABLE,
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

        char* name;

        struct {
            struct ASTNode* condition;
            struct ASTNode* then_branch;
            struct ASTNode* else_branch;
        } if_statement;

        struct {
            struct ASTNode* condition;
            struct ASTNode* body;
        } while_statement;

        struct {
            char* name;
            struct ASTNode* value;
        } assignment;

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
