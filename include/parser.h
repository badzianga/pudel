#pragma once
#include "lexer.h"

typedef enum ASTNodeType {
    AST_NODE_PROGRAM,
    AST_NODE_VARIABLE_DECLARATION,
    AST_NODE_EXPRESSION_STATEMENT,
    AST_NODE_PRINT_STATEMENT,
    AST_NODE_IF_STATEMENT,
    AST_NODE_WHILE_STATEMENT,
    AST_NODE_BLOCK,

    AST_NODE_ASSIGNMENT,
    AST_NODE_LOGICAL,
    AST_NODE_BINARY,
    AST_NODE_UNARY,
    AST_NODE_LITERAL,
    AST_NODE_VARIABLE,
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;

    union {
        // program, block
        struct {
            struct ASTNode** statements;
            int count;
            int capacity;
        } scope;

        // var decl
        struct {
            char* name;
            struct ASTNode* initializer;
        } variable_declaration;

        // expr stmt
        struct ASTNode* expression;

        // if
        struct {
            struct ASTNode* condition;
            struct ASTNode* then_branch;
            struct ASTNode* else_branch;
        } if_statement;

        // while
        struct {
            struct ASTNode* condition;
            struct ASTNode* body;
        } while_statement;

        // assignment
        struct {
            char* name;
            TokenType op;
            struct ASTNode* value;
        } assignment;

        // logical, binary
        struct {
            struct ASTNode* left;
            TokenType op;
            struct ASTNode* right;
        } binary;

        // unary
        struct {
            TokenType op;
            struct ASTNode* right;
        } unary;

        // literal
        int literal;

        // var
        char* name;
    };
} ASTNode;

ASTNode* parser_parse(TokenArray* token_array);
void parser_free_ast(ASTNode* root);
