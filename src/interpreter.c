#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interpreter.h"
#include "lexer.h"
#include "parser.h"
#include "value.h"

typedef struct Variable {
    char* name;
    Value value;
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

static bool is_truthy(Value value) {
    switch (value.type) {
        case VALUE_NULL: return false;
        case VALUE_NUMBER: return value.number != 0.0;
        case VALUE_BOOL: return value.boolean;
    }
    return false;
}

Value interpreter_interpret(ASTNode* root) {
    switch (root->type) {
        case AST_NODE_PROGRAM: {
            for (int i = 0; i < root->block.count; ++i) {
                interpreter_interpret(root->block.statements[i]);
            }
        } break;
        case AST_NODE_VARIABLE_DECLARATION: {
            if (get_variable(root->variable_declaration.name) != NULL) {
                fprintf(stderr, "error: redefinition of variable '%s'\n", root->variable_declaration.name);
                exit(1);
            }
            Value value = NULL_VALUE();
            if (root->variable_declaration.initializer != NULL) {
                value = interpreter_interpret(root->variable_declaration.initializer);
            }
            variables[variable_count++] = (Variable){ root->variable_declaration.name, value };
        } break;
        case AST_NODE_EXPRESSION_STATEMENT: {
            interpreter_interpret(root->expression);
        } break;
        case AST_NODE_PRINT_STATEMENT: {
            Value value = interpreter_interpret(root->expression);
            switch (value.type) {
                case VALUE_NULL: {
                    printf("null\n");
                } break;
                case VALUE_NUMBER: {
                    printf("%lf\n", value.number);
                } break;
                case VALUE_BOOL: {
                    printf("%s\n", value.boolean ? "true" : "false");
                } break;
                default: {
                    printf("interpreter::interpreter_interpret: invalid value type in print statement: %d\n", value.type);
                    exit(1);
                } break;
            }
        } break;
        case AST_NODE_IF_STATEMENT: {
            if (is_truthy(interpreter_interpret(root->if_statement.condition))) {
                interpreter_interpret(root->if_statement.then_branch);
            }
            else if (root->if_statement.else_branch != NULL) {
                interpreter_interpret(root->if_statement.else_branch);
            }
        } break;
        case AST_NODE_WHILE_STATEMENT: {
            while (is_truthy(interpreter_interpret(root->while_statement.condition))) {
                if (root->while_statement.body != NULL) {
                    interpreter_interpret(root->while_statement.body);
                }
            }
        } break;
        case AST_NODE_BLOCK: {
            for (int i = 0; i < root->block.count; ++i) {
                interpreter_interpret(root->block.statements[i]);
            }
        } break;
        case AST_NODE_ASSIGNMENT: {
            Variable* var = get_variable(root->assignment.name);
            if (var == NULL) {
                fprintf(stderr, "error: undeclared identifier '%s'\n", root->assignment.name);
                exit(1);
            }
            Value value = interpreter_interpret(root->assignment.value);
            // TODO: do these ops only for number values
            switch(root->assignment.op) {
                case TOKEN_PLUS_EQUAL: {
                    var->value.number += value.number;
                } break;
                case TOKEN_MINUS_EQUAL: {
                    var->value.number -= value.number;
                } break;
                case TOKEN_ASTERISK_EQUAL: {
                    var->value.number *= value.number;
                } break;
                case TOKEN_SLASH_EQUAL: {
                    // TODO: check division by zero
                    var->value.number /= value.number;
                } break;
                case TOKEN_EQUAL: {
                    var->value = value;
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
            return var->value;
        } break;
        case AST_NODE_LOGICAL: {
            Value left = interpreter_interpret(root->binary.left);
            switch (root->binary.op) {
                case TOKEN_OR: {
                    if (is_truthy(left)) return left;
                } break;
                case TOKEN_AND: {
                    if (!is_truthy(left)) return left;
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
            Value left = interpreter_interpret(root->binary.left);
            Value right = interpreter_interpret(root->binary.right);

            // TODO: do these ops only for proper value types
            switch (root->binary.op) {
                case TOKEN_PLUS:
                    return NUMBER_VALUE(left.number + right.number);
                case TOKEN_MINUS:
                    return NUMBER_VALUE(left.number - right.number);
                case TOKEN_ASTERISK:
                    return NUMBER_VALUE(left.number * right.number);
                case TOKEN_SLASH: {
                    // TODO: check division by zero
                    return NUMBER_VALUE(left.number / right.number);
                }
                case TOKEN_EQUAL_EQUAL:
                    return BOOL_VALUE(left.number == right.number);
                case TOKEN_NOT_EQUAL:
                    return BOOL_VALUE(left.number == right.number);
                case TOKEN_GREATER:
                    return BOOL_VALUE(left.number > right.number);
                case TOKEN_GREATER_EQUAL:
                    return BOOL_VALUE(left.number >= right.number);
                case TOKEN_LESS:
                    return BOOL_VALUE(left.number < right.number);
                case TOKEN_LESS_EQUAL:
                    return BOOL_VALUE(left.number <= right.number);
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
            Value value = interpreter_interpret(root->unary.right);

            // TODO: check types
            switch (root->unary.op) {
                case TOKEN_MINUS: {
                    value.number = -value.number;
                    return value;
                } break;
                case TOKEN_NOT: {
                    return BOOL_VALUE(!is_truthy(value));
                } break;
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
    return (Value) { 0 };
}
