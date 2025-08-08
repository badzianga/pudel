#include <stdarg.h>
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

static void runtime_error(const char* format, ...) {
    fputs("runtime error: ", stderr);
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    exit(1);
}

static void assert_number_operand(Value a) {
    if (IS_NUMBER(a)) return;
    runtime_error("operand must be a number");
}

static void assert_number_operands(Value a, Value b) {
    if (IS_NUMBER(a) && IS_NUMBER(b)) return;
    runtime_error("operands must be numbers");
}

static bool values_equal(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case VALUE_NULL: return true;
        case VALUE_NUMBER: return a.number == b.number;
        case VALUE_BOOL: return a.boolean == b.boolean;
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
                runtime_error("redeclaration of variable '%s'", root->variable_declaration.name);
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
                default: break;
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
                runtime_error("undeclared identifier '%s'", root->assignment.name);
            }
            Value value = interpreter_interpret(root->assignment.value);
            switch(root->assignment.op) {
                case TOKEN_PLUS_EQUAL: {
                    assert_number_operands(var->value, value);
                    var->value.number += value.number;
                } break;
                case TOKEN_MINUS_EQUAL: {
                    assert_number_operands(var->value, value);
                    var->value.number -= value.number;
                } break;
                case TOKEN_ASTERISK_EQUAL: {
                    assert_number_operands(var->value, value);
                    var->value.number *= value.number;
                } break;
                case TOKEN_SLASH_EQUAL: {
                    assert_number_operands(var->value, value);
                    // TODO: check division by zero
                    var->value.number /= value.number;
                } break;
                case TOKEN_EQUAL: {
                    var->value = value;
                } break;
                default: break;
            }
            return var->value;
        }
        case AST_NODE_LOGICAL: {
            Value left = interpreter_interpret(root->binary.left);
            switch (root->binary.op) {
                case TOKEN_OR: {
                    if (is_truthy(left)) return left;
                } break;
                case TOKEN_AND: {
                    if (!is_truthy(left)) return left;
                } break;
                default: break;
            }
            return interpreter_interpret(root->binary.right);
        }
        case AST_NODE_BINARY: {
            Value left = interpreter_interpret(root->binary.left);
            Value right = interpreter_interpret(root->binary.right);

            switch (root->binary.op) {
                case TOKEN_PLUS: {
                    assert_number_operands(left, right);
                    return NUMBER_VALUE(left.number + right.number);
                }
                case TOKEN_MINUS: {
                    assert_number_operands(left, right);
                    return NUMBER_VALUE(left.number - right.number);
                }
                case TOKEN_ASTERISK: {
                    assert_number_operands(left, right);
                    return NUMBER_VALUE(left.number * right.number);
                }
                case TOKEN_SLASH: {
                    assert_number_operands(left, right);
                    // TODO: check division by zero
                    return NUMBER_VALUE(left.number / right.number);
                }
                case TOKEN_EQUAL_EQUAL:
                    return BOOL_VALUE(values_equal(left, right));
                case TOKEN_NOT_EQUAL:
                    return BOOL_VALUE(!values_equal(left, right));
                case TOKEN_GREATER: {
                    assert_number_operands(left, right);
                    return BOOL_VALUE(left.number > right.number);
                }
                case TOKEN_GREATER_EQUAL: {
                    assert_number_operands(left, right);
                    return BOOL_VALUE(left.number >= right.number);
                }
                case TOKEN_LESS: {
                    assert_number_operands(left, right);
                    return BOOL_VALUE(left.number < right.number);
                }
                case TOKEN_LESS_EQUAL: {
                    assert_number_operands(left, right);
                    return BOOL_VALUE(left.number <= right.number);
                }
                default: break;
            }
        } break;
        case AST_NODE_UNARY: {
            Value value = interpreter_interpret(root->unary.right);

            switch (root->unary.op) {
                case TOKEN_MINUS: {
                    assert_number_operand(value);
                    value.number = -value.number;
                    return value;
                }
                case TOKEN_NOT: {
                    return BOOL_VALUE(!is_truthy(value));
                }
                default: break;
            }
            return NULL_VALUE();
        }
        case AST_NODE_LITERAL: {
            return root->literal;
        }
        case AST_NODE_VARIABLE: {
            Variable* var = get_variable(root->name);
            if (var == NULL) {
                runtime_error("undeclared identifier '%s'\n", root->name);
            }
            return var->value;
        }
    }
    return NULL_VALUE();
}
