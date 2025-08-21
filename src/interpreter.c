#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "environment.h"
#include "interpreter.h"
#include "lexer.h"
#include "memory.h"
#include "parser.h"
#include "value.h"

static Environment* global_scope = NULL;
static Environment* env = NULL;
static int function_depth = 0;

typedef enum {
    FLOW_NORMAL,
    FLOW_RETURN,
} FlowSignal;

typedef struct ControlContext {
    jmp_buf buf;
    FlowSignal signal;
    struct ControlContext* parent;
} ControlContext;

static ControlContext* current_context = NULL;
static Value ctx_return_value = NULL_VALUE();

static bool is_truthy(Value value) {
    switch (value.type) {
        case VALUE_NULL:   return false;
        case VALUE_NUMBER: return value.number != 0.0;
        case VALUE_BOOL:   return value.boolean;
        case VALUE_STRING: return value.string->length != 0;
        case VALUE_LIST:   return value.list->length != 0;
        case VALUE_NATIVE: return true;
        case VALUE_FUNCTION: return true;
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

static Value typeof_native(int argc, Value* argv) {
    if (argc != 1) runtime_error("expected 1 argument but got %d", argc);
    return STRING_VALUE(string_from(value_type_as_cstr(argv[0].type)));
}

static Value number_native(int argc, Value* argv) {
    if (argc != 1) runtime_error("expected 1 argument but got %d", argc);
    Value arg = argv[0];
    switch (arg.type) {
        case VALUE_NULL:   return NUMBER_VALUE(0.0);
        case VALUE_NUMBER: return arg;
        case VALUE_BOOL:   return NUMBER_VALUE(arg.boolean ? 1.0 : 0.0);
        case VALUE_STRING: return NUMBER_VALUE(strtod(arg.string->data, NULL));
        default: runtime_error("cannot convert from %s to number", value_type_as_cstr(arg.type));
    }
    return NULL_VALUE();
}

static Value bool_native(int argc, Value* argv) {
    if (argc != 1) runtime_error("expected 1 argument but got %d", argc);
    Value arg = argv[0];
    switch (arg.type) {
        case VALUE_NULL:     return BOOL_VALUE(false);
        case VALUE_NUMBER:   return BOOL_VALUE(arg.number != 0.0);
        case VALUE_BOOL:     return arg;
        case VALUE_STRING:   return BOOL_VALUE(arg.string->length != 0);
        case VALUE_NATIVE:   return BOOL_VALUE(true);
        case VALUE_FUNCTION: return BOOL_VALUE(true);
        default: runtime_error("cannot convert from %s to bool", value_type_as_cstr(arg.type));
    }
    return NULL_VALUE();
}

static Value string_native(int argc, Value* argv) {
    if (argc != 1) runtime_error("expected 1 argument but got %d", argc);
    Value arg = argv[0];
    switch (arg.type) {
        case VALUE_NULL:   return STRING_VALUE(string_from("null"));
        case VALUE_NUMBER: {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "%lf", arg.number);
            return STRING_VALUE(string_from(buffer));
        }
        case VALUE_BOOL:   return STRING_VALUE(string_from(arg.boolean ? "true" : "false"));
        case VALUE_STRING: return arg;
        case VALUE_NATIVE: return STRING_VALUE(string_from("<native function>")); //TODO: also print name of function
        case VALUE_FUNCTION: return STRING_VALUE(arg.function->name);
        default: runtime_error("cannot convert from %s to string", value_type_as_cstr(arg.type));
    }
    return NULL_VALUE();
}

static Value append_native(int argc, Value* argv) {
    if (argc != 2) runtime_error("expected 2 arguments but got %d", argc);
    Value list = argv[0];
    Value value = argv[1];

    if (list.list->capacity < list.list->length + 1) {
        list.list->capacity = GROW_CAPACITY(list.list->capacity);
        list.list->values = GROW_ARRAY(Value, list.list->values, list.list->capacity);
    }

    list.list->values[list.list->length++] = value;
    return NULL_VALUE();
}

static Value length_native(int argc, Value* argv) {
    if (argc != 1) runtime_error("expected 1 argument but got %d", argc);
    Value list = argv[0];
    return NUMBER_VALUE(list.list->length);
}

static void add_natives() {
    env_define(global_scope, string_from("clock"), NATIVE_VALUE(clock_native));
    env_define(global_scope, string_from("print"), NATIVE_VALUE(print_native));
    env_define(global_scope, string_from("input"), NATIVE_VALUE(input_native));
    env_define(global_scope, string_from("typeof"), NATIVE_VALUE(typeof_native));

    env_define(global_scope, string_from("number"), NATIVE_VALUE(number_native));
    env_define(global_scope, string_from("bool"), NATIVE_VALUE(bool_native));
    env_define(global_scope, string_from("string"), NATIVE_VALUE(string_native));

    env_define(global_scope, string_from("append"), NATIVE_VALUE(append_native));
    env_define(global_scope, string_from("length"), NATIVE_VALUE(length_native));
}

static Value evaluate(ASTNode* root);

static Value* evaluate_subscription(ASTNodeSubscription* node) {
    Value list = evaluate(node->expression);
    if (!IS_LIST(list)) {
        runtime_error("object is not subscriptable");
    }
    Value index = evaluate(node->index);
    if (!IS_NUMBER(index)) {
        runtime_error("list index must be a number");
    }
    int idx = (int)index.number;
    if (idx < 0 || idx >= list.list->length) {
        runtime_error("index out of range");
    }
    return &list.list->values[idx];
}

static Value evaluate(ASTNode* root) {
    switch (root->type) {
        case AST_NODE_PROGRAM: {
            ASTNodeBlock* block = (ASTNodeBlock*)root;
            for (int i = 0; i < block->count; ++i) {
                evaluate(block->statements[i]);
            }
        } break;
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
        case AST_NODE_FUNC_DECL: {
            ASTNodeFuncDecl* func_decl = (ASTNodeFuncDecl*)root;
            Function* function = malloc(sizeof(Function));
            // TODO: name might not be needed in function value
            function->name = func_decl->name;
            function->params = func_decl->params;
            function->param_count = func_decl->param_count;
            function->body = func_decl->body;

            env_define(global_scope, function->name, FUNCTION_VALUE(function));
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
        case AST_NODE_RETURN_STMT: {
            ASTNodeExprStmt* return_stmt = (ASTNodeExprStmt*)root;

            if (function_depth <= 0) runtime_error("return is only possible from functions");

            Value return_value = (return_stmt->expression != NULL) ? evaluate(return_stmt->expression) : NULL_VALUE();
            ctx_return_value = return_value;
            current_context->signal = FLOW_RETURN;
            longjmp(current_context->buf, 1);
        } break;
        case AST_NODE_ASSIGNMENT: {
            ASTNodeAssignment* assignment = (ASTNodeAssignment*)root;
            Value* var = NULL;
            if (assignment->target->type == AST_NODE_VAR) {
                ASTNodeVar* target = (ASTNodeVar*)assignment->target;
                var = env_get_ref(env, target->name);
                if (var == NULL) {
                    runtime_error("undeclared identifier '%s'", target->name->data);
                }
            }
            else if (assignment->target->type == AST_NODE_SUBSCRIPTION) {
                var = evaluate_subscription((ASTNodeSubscription*)assignment->target);
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
        case AST_NODE_TERNARY: {
            ASTNodeIfStmt* ternary = (ASTNodeIfStmt*)root;
            if (is_truthy(evaluate(ternary->condition))) {
                return evaluate(ternary->then_branch);
            }
            return evaluate(ternary->else_branch);
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

            if (callee.type == VALUE_NATIVE) {
                Value* args = malloc(sizeof(Value) * call->count);
                for (int i = 0; i < call->count; ++i) {
                    args[i] = evaluate(call->arguments[i]);
                }
                Value result = callee.native(call->count, args);
                free(args);
                return result;
            }
            if (callee.type == VALUE_FUNCTION) {
                if (callee.function->param_count != call->count) {
                    runtime_error("expected %d arguments, but got %d", callee.function->param_count, call->count);
                }
                Environment* tmp = env;
                Environment* func_scope = env_new_with_enclosing(global_scope);
                for (int i = 0; i < call->count; ++i) {
                    env_define(func_scope, callee.function->params[i], evaluate(call->arguments[i]));
                }
                env = func_scope;

                ControlContext ctx = { 0 };
                ctx.parent = current_context;
                current_context = &ctx;
                Value return_value = NULL_VALUE();

                ++function_depth;

                if (setjmp(ctx.buf) == 0) {
                    evaluate(callee.function->body);
                }
                else {
                    FlowSignal sig = ctx.signal;
                    if (sig == FLOW_RETURN) {
                        return_value = ctx_return_value;
                    }
                }

                --function_depth;

                current_context = ctx.parent;
                env = tmp;
                free(func_scope);
                return return_value;
                
            }
            else {
                runtime_error("attempt to call a non-function value");
            }
        } break;
        case AST_NODE_SUBSCRIPTION: {
            ASTNodeSubscription* subscription = (ASTNodeSubscription*)root;
            return *evaluate_subscription(subscription);
        } break;
        case AST_NODE_LITERAL: {
            ASTNodeLiteral* literal = (ASTNodeLiteral*)root;
            return literal->value;
        }
        case AST_NODE_VAR: {
            ASTNodeVar* var = (ASTNodeVar*)root;
            Value* variable = env_get_ref(env, var->name);
            if (variable == NULL) {
                runtime_error("undeclared identifier '%s'", var->name->data);
            }
            return *variable;
        }
        case AST_NODE_LIST: {
            ASTNodeList* list_node = (ASTNodeList*)root;
            List* list = list_new(list_node->count);
            list->length = list_node->count;
            for (int i = 0; i < list_node->count; ++i) {
                list->values[i] = evaluate(list_node->expressions[i]);
            }
            return LIST_VALUE(list);
        }
    }
    return NULL_VALUE();
}

Value interpreter_interpret(ASTNode* root) {
    global_scope = env_new();
    env = global_scope;
    add_natives();

    Value value = evaluate(root);
    env_free(env);
    return value;
}
