#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interpreter.h"
#include "lexer.h"
#include "parser.h"

#define INTERPRET_BINARY(node, op) interpreter_interpret((node)->binary.left) op interpreter_interpret((node)->binary.right)
#define INTERPRET_UNARY(node, op) op interpreter_interpret((node)->unary.right)
#define INTERPRET_ASSIGNMENT(var, op, val) (var)->value op val 

typedef struct Variable {
    char* name;
    int value;
} Variable;

static Variable variables[128] = { 0 };
static int variable_count = 0;

static Variable* get_variable(char* name) {
    Variable* end = variables + variable_count;
    for (Variable* var = variables; var != end; ++var) {        
        if (strcmp(name, var->name) == 0) {
            return var;
        }
    }
    return NULL;
}

int interpreter_interpret(ASTNode* root) {
    switch (root->type) {
        case AST_NODE_PROGRAM: {
            for (int i = 0; i < root->scope.count; ++i) {
                interpreter_interpret(root->scope.statements[i]);
            }
        } break;
        case AST_NODE_VARIABLE_DECLARATION: {
            if (get_variable(root->variable_declaration.name) != NULL) {
                fprintf(stderr, "error: redefinition of variable '%s'\n", root->variable_declaration.name);
                exit(1);
            }
            int value = 0;
            if (root->variable_declaration.initializer != NULL) {
                value = interpreter_interpret(root->variable_declaration.initializer);
            }
            variables[variable_count++] = (Variable) { root->variable_declaration.name, value };
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
        case AST_NODE_WHILE_STATEMENT: {
            while (interpreter_interpret(root->while_statement.condition)) {
                if (root->while_statement.body != NULL) {
                    interpreter_interpret(root->while_statement.body);
                }
            }
        } break;
        case AST_NODE_BLOCK: {
            for (int i = 0; i < root->scope.count; ++i) {
                interpreter_interpret(root->scope.statements[i]);
            }
        } break;
        case AST_NODE_ASSIGNMENT: {
            Variable* var = get_variable(root->assignment.name);
            if (var == NULL) {
                fprintf(stderr, "error: undeclared identifier '%s'\n", root->assignment.name);
                exit(1);
            }
            int value = interpreter_interpret(root->assignment.value);
            switch(root->assignment.op) {
                case TOKEN_PLUS_EQUAL: {
                    return INTERPRET_ASSIGNMENT(var, +=, value);
                } break;
                case TOKEN_MINUS_EQUAL: {
                    return INTERPRET_ASSIGNMENT(var, -=, value);
                } break;
                case TOKEN_ASTERISK_EQUAL: {
                    return INTERPRET_ASSIGNMENT(var, *=, value);
                } break;
                case TOKEN_SLASH_EQUAL: {
                    return INTERPRET_ASSIGNMENT(var, /=, value);
                } break;
                case TOKEN_EQUAL: {
                    return INTERPRET_ASSIGNMENT(var, =, value);
                } break;
                default: {
                    fprintf(
                        stderr,
                        "error: invalid operator in assignment operation: '%s'\n",
                        token_as_cstr(root->assignment.op)
                    );
                    exit(1);
                } break;
            }
        } break;
        case AST_NODE_LOGICAL: {
            int left = interpreter_interpret(root->binary.left);
            switch (root->binary.op) {
                case TOKEN_LOGICAL_OR: {
                    if (left) return left;
                } break;
                case TOKEN_LOGICAL_AND: {
                    if (!left) return left;
                } break;
                default: {
                    fprintf(
                        stderr,
                        "error: invalid operator in logical operation: '%s'\n",
                        token_as_cstr(root->binary.op)
                    );
                    exit(1);
                } break;
            }
            return interpreter_interpret(root->binary.right);
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
        } break;
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
        } break;
        case AST_NODE_LITERAL: {
            return root->literal;
        } break;
        case AST_NODE_VARIABLE: {
            Variable* var = get_variable(root->name);
            if (var == NULL) {
                fprintf(stderr, "error: undeclared identifier '%s'\n", root->name);
                exit(1);
            }
            return var->value;
        } break;
        default: {
            fprintf(
                stderr,
                "interpreter::interpreter_interpret: unknown AST node type with value: %d\n",
                root->type
            );
            exit(1);
        } break;
    }
    return 0;
}
