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
        case VALUE_STRING: return value.string->length != 0;
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

Value evaluate(ASTNode* root) {
    switch (root->type) {
        case AST_NODE_PROGRAM:
        case AST_NODE_BLOCK: {
            ASTNodeBlock* block = (ASTNodeBlock*)root;
            for (int i = 0; i < block->count; ++i) {
                evaluate(block->statements[i]);
            }
        } break;
        case AST_NODE_VAR_DECL: {
            ASTNodeVarDecl* var_decl = (ASTNodeVarDecl*)root;
            if (get_variable(var_decl->name) != NULL) {
                runtime_error("redeclaration of variable '%s'", var_decl->name);
            }
            Value value = NULL_VALUE();
            if (var_decl->initializer != NULL) {
                value = evaluate(var_decl->initializer);
            }
            variables[variable_count++] = (Variable){ .name = var_decl->name, .value = value };
        } break;
        case AST_NODE_EXPR_STMT: {
            ASTNodeExprStmt* expr_stmt = (ASTNodeExprStmt*)root;
            evaluate(expr_stmt->expression);
        } break;
        case AST_NODE_PRINT_STMT: {
            ASTNodeExprStmt* print_stmt = (ASTNodeExprStmt*)root;
            Value value = evaluate(print_stmt->expression);
            print_value(value);
            fputs("\n", stdout);
        } break;
        case AST_NODE_IF_STMT: {
            ASTNodeIfStmt* if_stmt = (ASTNodeIfStmt*)root;
            if (is_truthy(evaluate(if_stmt->condition))) {
                evaluate(if_stmt->then_branch);
            }
            else if (if_stmt->else_branch != NULL) {
                evaluate(if_stmt->else_branch);
            }
        } break;
        case AST_NODE_WHILE_STMT: {
            ASTNodeWhileStmt* while_stmt = (ASTNodeWhileStmt*)root;
            while (is_truthy(evaluate(while_stmt->condition))) {
                if (while_stmt->body != NULL) {
                    evaluate(while_stmt->body);
                }
            }
        } break;
        case AST_NODE_ASSIGNMENT: {
            ASTNodeAssignment* assignment = (ASTNodeAssignment*)root;
            Variable* var = get_variable(assignment->name);
            if (var == NULL) {
                runtime_error("undeclared identifier '%s'", assignment->name);
            }
            Value value = evaluate(assignment->value);
            switch(assignment->op) {
                case TOKEN_PLUS_EQUAL: {
                    if (IS_NUMBER(var->value) && IS_NUMBER(value)) {
                        var->value.number += value.number;
                    }
                    else if (IS_STRING(var->value) && IS_STRING(value)) {
                        var->value.string = string_concat(var->value.string, value.string);
                    }
                    else {
                        runtime_error("invalid operands for '+=' operation");
                    }
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
            Value left = evaluate(binary->left);
            switch (binary->op) {
                case TOKEN_OR: {
                    if (is_truthy(left)) return left;
                } break;
                case TOKEN_AND: {
                    if (!is_truthy(left)) return left;
                } break;
                default: break;
            }
            return evaluate(binary->right);
        }
        case AST_NODE_BINARY: {
            ASTNodeBinary* binary = (ASTNodeBinary*)root;
            Value left = evaluate(binary->left);
            Value right = evaluate(binary->right);

            switch (binary->op) {
                case TOKEN_PLUS: {
                    if (IS_NUMBER(left) && IS_NUMBER(right)) {
                        return NUMBER_VALUE(left.number + right.number);
                    }
                    else if (IS_STRING(left) && IS_STRING(right)) {
                        return STRING_VALUE(string_concat(left.string, right.string));
                    }
                    else {
                        runtime_error("invalid operands for '+' operation");
                    }
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
            Value value = evaluate(unary->right);

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

Value interpreter_interpret(ASTNode* root) {
    return evaluate(root);
}
