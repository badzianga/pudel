#include <stdio.h>
#include <stdlib.h>
#include "interpreter.h"
#include "lexer.h"
#include "parser.h"

#define INTERPRET_BINARY(node, op) interpreter_interpret((node)->binary.left) op interpreter_interpret((node)->binary.right)
#define INTERPRET_UNARY(node, op) op interpreter_interpret((node)->unary.right)

int interpreter_interpret(ASTNode* root) {
    switch (root->type) {
        case AST_NODE_PROGRAM: {
            for (int i = 0; i < root->scope.count; ++i) {
                interpreter_interpret(root->scope.statements[i]);
            }
        } break;
        case AST_NODE_EXPRESSION_STATEMENT: {
            interpreter_interpret(root->expression);
        } break;
        case AST_NODE_PRINT_STATEMENT: {
            printf("%d\n", interpreter_interpret(root->expression));
        } break;
        case AST_NODE_IF_STATEMENT: {
            if (interpreter_interpret(root->if_statement.condition)) {
                interpreter_interpret(root->if_statement.then_branch);
            }
            else if (root->if_statement.else_branch != NULL) {
                interpreter_interpret(root->if_statement.else_branch);
            }
        } break;
        case AST_NODE_BLOCK: {
            for (int i = 0; i < root->scope.count; ++i) {
                interpreter_interpret(root->scope.statements[i]);
            }
        } break;
        case AST_NODE_BINARY: {
            switch (root->binary.op) {
                case TOKEN_PLUS:          return INTERPRET_BINARY(root, +);
                case TOKEN_MINUS:         return INTERPRET_BINARY(root, -);
                case TOKEN_ASTERISK:      return INTERPRET_BINARY(root, *);
                case TOKEN_SLASH: {
                    int left = interpreter_interpret((root)->binary.left);
                    int right = interpreter_interpret((root)->binary.right);
                    if (right == 0) {
                        fprintf(stderr, "error: division by zero\n");
                        exit(1);
                    }
                    return left / right;
                }
                case TOKEN_EQUAL_EQUAL:   return INTERPRET_BINARY(root, ==);
                case TOKEN_NOT_EQUAL:     return INTERPRET_BINARY(root, !=);
                case TOKEN_GREATER:       return INTERPRET_BINARY(root, >);
                case TOKEN_GREATER_EQUAL: return INTERPRET_BINARY(root, >=);
                case TOKEN_LESS:          return INTERPRET_BINARY(root, <);
                case TOKEN_LESS_EQUAL:    return INTERPRET_BINARY(root, <=);
                default: {
                    fprintf(
                        stderr,
                        "error: invalid operator in binary operation: '%s'\n",
                        token_as_cstr(root->binary.op)
                    );
                    exit(1);
                }
            }
        }
        case AST_NODE_UNARY: {
            switch (root->unary.op) {
                case TOKEN_MINUS: return INTERPRET_UNARY(root, -);
                case TOKEN_NOT:   return INTERPRET_UNARY(root, !);
                default: {
                    fprintf(
                        stderr,
                        "error: invalid operator in unary operation: '%s'\n",
                        token_as_cstr(root->unary.op)
                    );
                    exit(1);
                }
            }
        }
        case AST_NODE_LITERAL: {
            return root->literal;
        }
        default: {
            fprintf(
                stderr,
                "interpreter::interpreter_interpret: unknown AST node type with value: %d\n",
                root->type
            );
        } break;
    }
    return 0;
}
