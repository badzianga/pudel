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
        case AST_NODE_PROGRAM:
        case AST_NODE_BLOCK: {
            ASTNodeBlock* block = (ASTNodeBlock*)root;
            for (int i = 0; i < block->count; ++i) {
                interpreter_interpret(block->statements[i]);
            }
        } break;
        case AST_NODE_VAR_DECL: {
            ASTNodeVarDecl* var_decl = (ASTNodeVarDecl*)root;
            if (get_variable(var_decl->name) != NULL) {
                runtime_error("redeclaration of variable '%s'", var_decl->name);
            }
            Value value = NULL_VALUE();
            if (var_decl->initializer != NULL) {
                value = interpreter_interpret(var_decl->initializer);
            }
            variables[variable_count++] = (Variable){ .name = var_decl->name, .value = value };
        } break;
        case AST_NODE_EXPR_STMT: {
            ASTNodeExprStmt* expr_stmt = (ASTNodeExprStmt*)root;
            interpreter_interpret(expr_stmt->expression);
        } break;
        case AST_NODE_PRINT_STMT: {
            ASTNodeExprStmt* print_stmt = (ASTNodeExprStmt*)root;
            Value value = interpreter_interpret(print_stmt->expression);
            print_value(value);
            fputs("\n", stdout);
        } break;
        case AST_NODE_IF_STMT: {
            ASTNodeIfStmt* if_stmt = (ASTNodeIfStmt*)root;
            if (is_truthy(interpreter_interpret(if_stmt->condition))) {
                interpreter_interpret(if_stmt->then_branch);
            }
            else if (if_stmt->else_branch != NULL) {
                interpreter_interpret(if_stmt->else_branch);
            }
        } break;
        case AST_NODE_WHILE_STMT: {
            ASTNodeWhileStmt* while_stmt = (ASTNodeWhileStmt*)root;
            while (is_truthy(interpreter_interpret(while_stmt->condition))) {
                if (while_stmt->body != NULL) {
                    interpreter_interpret(while_stmt->body);
                }
            }
        } break;
        case AST_NODE_ASSIGNMENT: {
            ASTNodeAssignment* assignment = (ASTNodeAssignment*)root;
            Variable* var = get_variable(assignment->name);
            if (var == NULL) {
                runtime_error("undeclared identifier '%s'", assignment->name);
            }
            Value value = interpreter_interpret(assignment->value);
            switch(assignment->op) {
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
                    if (value.number == 0.0) runtime_error("cannot divide by zero");
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
            ASTNodeBinary* binary = (ASTNodeBinary*)root;
            Value left = interpreter_interpret(binary->left);
            switch (binary->op) {
                case TOKEN_OR: {
                    if (is_truthy(left)) return left;
                } break;
                case TOKEN_AND: {
                    if (!is_truthy(left)) return left;
                } break;
                default: break;
            }
            return interpreter_interpret(binary->right);
        }
        case AST_NODE_BINARY: {
            ASTNodeBinary* binary = (ASTNodeBinary*)root;
            Value left = interpreter_interpret(binary->left);
            Value right = interpreter_interpret(binary->right);

            switch (binary->op) {
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
                    if (right.number == 0.0) runtime_error("cannot divide by zero");
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
            ASTNodeUnary* unary = (ASTNodeUnary*)root;
            Value value = interpreter_interpret(unary->right);

            switch (unary->op) {
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
            ASTNodeLiteral* literal = (ASTNodeLiteral*)root;
            return literal->value;
        }
        case AST_NODE_VAR: {
            ASTNodeVar* var = (ASTNodeVar*)root;
            Variable* variable = get_variable(var->name);
            if (variable == NULL) {
                runtime_error("undeclared identifier '%s'\n", var->name);
            }
            return variable->value;
        }
    }
    return NULL_VALUE();
}
