#include <stdio.h>
#include <stdlib.h>
#include "debug.h"
#include "lexer.h"
#include "parser.h"

void debug_print_token_array(TokenArray* token_array) {
    const Token* end = token_array->tokens + token_array->count;
    for (const Token* token = token_array->tokens; token != end; ++token) {
        TokenType type = token->type;

        if (type == TOKEN_INT_LITERAL) {
            printf(
                "Line: %d,\ttoken: %s,\tvalue: %.*s\n",
                token->line,
                token_as_cstr(type),
                token->length,
                token->value
            );
        }
        else {
            printf(
                "Line: %d,\ttoken: %s\n",
                token->line,
                token_as_cstr(type)
            );
        }
    }
}

void debug_print_ast(ASTNode* root, int indent) {
    for (int i = 0; i < indent; ++i) printf("  ");

    switch (root->type) {
        case AST_NODE_PROGRAM: {
            printf("Program:\n");
            for (int i = 0; i < root->scope.count; ++i) {
                debug_print_ast(root->scope.statements[i], indent + 1);
            }
        } break;
        case AST_NODE_VARIABLE_DECLARATION: {
            printf("VarDecl: %s\n", root->assignment.name);
            if (root->assignment.value != NULL) {
                debug_print_ast(root->assignment.value, indent + 1);
            }
        } break;
        case AST_NODE_EXPRESSION_STATEMENT: {
            printf("Expression:\n");
            debug_print_ast(root->expression, indent + 1);
        } break;
        case AST_NODE_PRINT_STATEMENT: {
            printf("Print:\n");
            debug_print_ast(root->expression, indent + 1);
        } break;
        case AST_NODE_IF_STATEMENT: {
            printf("If:\n");
            debug_print_ast(root->if_statement.condition, indent + 1);
            for (int i = 0; i < indent; ++i) printf("  ");
            printf("Then:\n");
            debug_print_ast(root->if_statement.then_branch, indent + 1);
            if (root->if_statement.else_branch != NULL) {
                for (int i = 0; i < indent; ++i) printf("  ");
                printf("Else:\n");
                debug_print_ast(root->if_statement.else_branch, indent + 1);
            }
        } break;
        case AST_NODE_WHILE_STATEMENT: {
            printf("While:\n");
            debug_print_ast(root->while_statement.condition, indent + 1);
            for (int i = 0; i < indent; ++i) printf("  ");
            if (root->while_statement.body != NULL) {
                printf("Then:\n");
                debug_print_ast(root->while_statement.body, indent + 1);
            }
        } break;
        case AST_NODE_BLOCK: {
            printf("Block:\n");
            for (int i = 0; i < root->scope.count; ++i) {
                debug_print_ast(root->scope.statements[i], indent + 1);
            }
        } break;
        case AST_NODE_ASSIGNMENT: {
            printf("Assignment: %s\n", root->assignment.name);
            debug_print_ast(root->assignment.value, indent + 1);
        } break;
        case AST_NODE_LOGICAL: {
            printf("Logical: %s\n", token_as_cstr(root->binary.op));
            debug_print_ast(root->binary.left, indent + 1);
            debug_print_ast(root->binary.right, indent + 1);
        } break;
        case AST_NODE_BINARY: {
            printf("Binary: %s\n", token_as_cstr(root->binary.op));
            debug_print_ast(root->binary.left, indent + 1);
            debug_print_ast(root->binary.right, indent + 1);
        } break;
        case AST_NODE_UNARY: {
            printf("Unary: %s\n", token_as_cstr(root->unary.op));
            debug_print_ast(root->unary.right, indent + 1);
        } break;
        case AST_NODE_LITERAL: {
            printf("Literal: %d\n", root->literal);
        } break;
        case AST_NODE_VARIABLE: {
            printf("Variable: %s\n", root->name);
        } break;
        default: {
            fprintf(stderr, "debug::debug_print_ast: uknown AST node type with value: %d\n", root->type);
            exit(1);
        } break;
    }
}
