#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "environment.h"
#include "interpreter.h"
#include "lexer.h"
#include "parser.h"
#include "value.h"

static Environment* env = NULL;

static bool is_truthy(Value value) {
    switch (value.type) {
        case VALUE_NULL: return false;
        case VALUE_NUMBER: return value.number != 0.0;
        case VALUE_BOOL: return value.boolean;
        case VALUE_STRING: return value.string->length != 0;
        case VALUE_NATIVE: return true;
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

static Value clock_native(int argc, Value* argv) {
    (void)argv;
    if (argc != 0) runtime_error("expected 0 arguments but got %d", argc);
    return NUMBER_VALUE((double)clock() / CLOCKS_PER_SEC);
}

static Value print_native(int argc, Value* argv) {
    for (int i = 0; i < argc; ++i) {
        print_value(argv[i]);
    }
    fputc('\n', stdout);
    return NULL_VALUE();
}

static Value input_native(int argc, Value* argv) {
    if (argc > 1) runtime_error("expected 0 or 1 argument but got %d", argc);
    else if (argc == 1) print_value(argv[0]);
    char buffer[1024];
    if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';
        return STRING_VALUE(string_from(buffer));
    }
    runtime_error("failed to read from input");
    return NULL_VALUE();
}

static void add_natives(Environment* global_scope) {
    env_define(global_scope, string_from("clock"), NATIVE_VALUE(clock_native));
    env_define(global_scope, string_from("print"), NATIVE_VALUE(print_native));
    env_define(global_scope, string_from("input"), NATIVE_VALUE(input_native));
}

Value evaluate(ASTNode* root) {
    switch (root->type) {
        case AST_NODE_PROGRAM:
        case AST_NODE_BLOCK: {
            ASTNodeBlock* block = (ASTNodeBlock*)root;
            Environment* previous = env;
            env = env_new_with_enclosing(previous);
            for (int i = 0; i < block->count; ++i) {
                evaluate(block->statements[i]);
            }
            env_free(env);
            env = previous;
        } break;
        case AST_NODE_VAR_DECL: {
            ASTNodeVarDecl* var_decl = (ASTNodeVarDecl*)root;
            Value value = NULL_VALUE();
            if (var_decl->initializer != NULL) {
                value = evaluate(var_decl->initializer);
            }
            if (env_define(env, var_decl->name, value)) {
                runtime_error("redeclaration of variable '%s'", var_decl->name->data);
            }
        } break;
        case AST_NODE_EXPR_STMT: {
            ASTNodeExprStmt* expr_stmt = (ASTNodeExprStmt*)root;
            evaluate(expr_stmt->expression);
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
            Value* var = env_get_ref(env, assignment->name);
            if (var == NULL) {
                runtime_error("undeclared identifier '%s'", assignment->name->data);
            }
            Value value = evaluate(assignment->value);
            switch(assignment->op) {
                case TOKEN_PLUS_EQUAL: {
                    if (IS_NUMBER(*var) && IS_NUMBER(value)) {
                        var->number += value.number;
                    }
                    else if (IS_STRING(*var) && IS_STRING(value)) {
                        var->string = string_concat(var->string, value.string);
                    }
                    else {
                        runtime_error("invalid operands for '+=' operation");
                    }
                } break;
                case TOKEN_MINUS_EQUAL: {
                    assert_number_operands(*var, value);
                    var->number -= value.number;
                } break;
                case TOKEN_ASTERISK_EQUAL: {
                    assert_number_operands(*var, value);
                    var->number *= value.number;
                } break;
                case TOKEN_SLASH_EQUAL: {
                    assert_number_operands(*var, value);
                    if (value.number == 0.0) runtime_error("cannot divide by zero");
                    var->number /= value.number;
                } break;
                case TOKEN_EQUAL: {
                    *var = value;
                } break;
                default: break;
            }
            return *var;
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
        case AST_NODE_CALL: {
            ASTNodeCall* call = (ASTNodeCall*)root;
            Value callee = evaluate(call->callee);

            if (callee.type != VALUE_NATIVE) {
                runtime_error("attempt to call a non-function value");
            }

            Value* args = malloc(sizeof(Value) * call->count);
            for (int i = 0; i < call->count; ++i) {
                args[i] = evaluate(call->arguments[i]);
            }
            Value result = callee.native(call->count, args);
            free(args);
            return result;
        } break;
        case AST_NODE_LITERAL: {
            ASTNodeLiteral* literal = (ASTNodeLiteral*)root;
            return literal->value;
        }
        case AST_NODE_VAR: {
            ASTNodeVar* var = (ASTNodeVar*)root;
            Value* variable = env_get_ref(env, var->name);
            if (variable == NULL) {
                runtime_error("undeclared identifier '%s'\n", var->name->data);
            }
            return *variable;
        }
    }
    return NULL_VALUE();
}

Value interpreter_interpret(ASTNode* root) {
    env = env_new();
    add_natives(env);

    Value value = evaluate(root);
    env_free(env);
    return value;
}
