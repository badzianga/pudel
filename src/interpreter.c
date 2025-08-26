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

typedef enum {
    CTX_OTHER,
    CTX_FUNCTION,
    CTX_LOOP,
} ContextType;

typedef enum {
    FLOW_NORMAL,
    FLOW_RETURN,
    FLOW_BREAK,
    FLOW_CONTINUE,
} FlowSignal;

typedef struct ControlContext {
    jmp_buf buf;
    ContextType type;
    FlowSignal signal;
    struct ControlContext* parent;
} ControlContext;

static ControlContext* current_context = NULL;
static Value ctx_return_value = NULL_VALUE();

static bool is_truthy(Value value) {
    switch (value.type) {
        case VALUE_NULL:   return false;
        case VALUE_INT: return value.integer != 0;
        case VALUE_FLOAT: return value.floating != 0.0;
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

static Value clock_native(int argc, Value* argv) {
    (void)argv;
    if (argc != 0) runtime_error("expected 0 arguments but got %d", argc);
    return FLOAT_VALUE((double)clock() / CLOCKS_PER_SEC);
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

static Value int_native(int argc, Value* argv) {
    if (argc != 1) runtime_error("expected 1 argument but got %d", argc);
    Value arg = argv[0];
    switch (arg.type) {
        case VALUE_NULL:   return INT_VALUE(0);
        case VALUE_INT:    return arg;
        case VALUE_FLOAT:  return INT_VALUE((int64_t)arg.floating);
        case VALUE_BOOL:   return INT_VALUE(arg.boolean ? 1 : 0);
        case VALUE_STRING: return INT_VALUE(strtoll(arg.string->data, NULL, 10));
        default: runtime_error("cannot convert from %s to int", value_type_as_cstr(arg.type));
    }
    return NULL_VALUE();
}

static Value float_native(int argc, Value* argv) {
    if (argc != 1) runtime_error("expected 1 argument but got %d", argc);
    Value arg = argv[0];
    switch (arg.type) {
        case VALUE_NULL:   return FLOAT_VALUE(0.0);
        case VALUE_INT:    return FLOAT_VALUE((double)arg.integer);
        case VALUE_FLOAT:  return arg;
        case VALUE_BOOL:   return FLOAT_VALUE(arg.boolean ? 1.0 : 0.0);
        case VALUE_STRING: return FLOAT_VALUE(strtod(arg.string->data, NULL));
        default: runtime_error("cannot convert from %s to float", value_type_as_cstr(arg.type));
    }
    return NULL_VALUE();
}

static Value bool_native(int argc, Value* argv) {
    if (argc != 1) runtime_error("expected 1 argument but got %d", argc);
    Value arg = argv[0];
    switch (arg.type) {
        case VALUE_NULL:     return BOOL_VALUE(false);
        case VALUE_INT:      return BOOL_VALUE(arg.integer != 0);
        case VALUE_FLOAT:    return FLOAT_VALUE(arg.floating != 0.0);
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
        case VALUE_INT: {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "%ld", arg.integer);
            return STRING_VALUE(string_from(buffer));
        }
        case VALUE_FLOAT: {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "%g", arg.floating);
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
    return INT_VALUE(list.list->length);
}

static void add_natives() {
    env_define(global_scope, string_from("clock"), NATIVE_VALUE(clock_native));
    env_define(global_scope, string_from("print"), NATIVE_VALUE(print_native));
    env_define(global_scope, string_from("input"), NATIVE_VALUE(input_native));
    env_define(global_scope, string_from("typeof"), NATIVE_VALUE(typeof_native));

    env_define(global_scope, string_from("int"), NATIVE_VALUE(int_native));
    env_define(global_scope, string_from("float"), NATIVE_VALUE(float_native));
    env_define(global_scope, string_from("bool"), NATIVE_VALUE(bool_native));
    env_define(global_scope, string_from("string"), NATIVE_VALUE(string_native));

    env_define(global_scope, string_from("append"), NATIVE_VALUE(append_native));
    env_define(global_scope, string_from("length"), NATIVE_VALUE(length_native));
}

static Value promote(Value value, ValueType target_type) {
    Value result = { .type = target_type };
    switch (target_type) {
        case VALUE_INT: {
            switch (value.type) {
                case VALUE_INT:   return value;
                case VALUE_FLOAT: result.integer = (int64_t)value.integer; break;
                case VALUE_BOOL:  result.integer = value.boolean ? 1 : 0; break;
                default: {
                    runtime_error(
                        "cannot promote value type from %s to %s",
                        value_type_as_cstr(value.type),
                        value_type_as_cstr(target_type)
                    );
                } break;
            }
        } break;
        case VALUE_FLOAT: {
            switch (value.type) {
                case VALUE_INT:   result.floating = (double)value.integer; break;
                case VALUE_FLOAT: return value;
                case VALUE_BOOL:  result.floating = value.boolean ? 1.0 : 0.0; break;
                default: {
                    runtime_error(
                        "cannot promote value type from %s to %s",
                        value_type_as_cstr(value.type),
                        value_type_as_cstr(target_type)
                    );
                } break;
            }
        } break;
        case VALUE_BOOL: {
            switch (value.type) {
                case VALUE_INT:   result.boolean = value.integer != 0; break;
                case VALUE_FLOAT: result.boolean = value.floating != 0.0; break;
                case VALUE_BOOL:  return value;
                default: {
                    runtime_error(
                        "cannot promote value type from %s to %s",
                        value_type_as_cstr(value.type),
                        value_type_as_cstr(target_type)
                    );
                } break;
            }
        } break;
        default: {
            runtime_error(
                "cannot promote value type from %s to %s",
                value_type_as_cstr(value.type),
                value_type_as_cstr(target_type)
            );
        } break;
    }
    return result;
}

static Value evaluate(ASTNode* root);

static Value* evaluate_subscription(ASTNodeSubscription* node) {
    Value list = evaluate(node->expression);
    if (!IS_LIST(list)) {
        runtime_error("object is not subscriptable");
    }
    Value index = evaluate(node->index);
    if (!IS_INT(index)) {
        runtime_error("list index must be an integer");
    }
    int idx = (int)index.integer;
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
            Function* function = malloc(sizeof(Function));  // FIXME: not freed
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

            ControlContext ctx = { 0 };
            ctx.parent = current_context;
            ctx.type = CTX_LOOP;
            current_context = &ctx;

            while (is_truthy(evaluate(while_stmt->condition))) {
                if (setjmp(ctx.buf) == 0) {
                    if (while_stmt->body != NULL) {
                        evaluate(while_stmt->body);
                    }
                }
                else {
                    FlowSignal sig = ctx.signal;
                    if (sig == FLOW_BREAK) break;
                    if (sig == FLOW_CONTINUE) continue;
                }
            }

            current_context = ctx.parent;
        } break;
        case AST_NODE_FOR_STMT: {
            ASTNodeForStmt* for_stmt = (ASTNodeForStmt*)root;

            if (for_stmt->initializer != NULL) {
                evaluate(for_stmt->initializer);
            }

            ControlContext ctx = { 0 };
            ctx.parent = current_context;
            ctx.type = CTX_LOOP;
            current_context = &ctx;

            while (is_truthy(evaluate(for_stmt->condition))) {
                if (setjmp(ctx.buf) == 0) {
                    if (for_stmt->body != NULL) {
                        evaluate(for_stmt->body);
                    }
                    if (for_stmt->increment != NULL) {
                        evaluate(for_stmt->increment);
                    }
                }
                else {
                    FlowSignal sig = ctx.signal;
                    if (sig == FLOW_BREAK) break;
                    if (sig == FLOW_CONTINUE) {
                        if (for_stmt->increment != NULL) {
                            evaluate(for_stmt->increment);
                        }
                        continue;   
                    }
                }
            }
        } break;
        case AST_NODE_RETURN_STMT: {
            ASTNodeExprStmt* return_stmt = (ASTNodeExprStmt*)root;

            if (current_context == NULL || current_context->type != CTX_FUNCTION) {
                runtime_error("'return' is only allowed inside functions");
            }

            Value return_value = (return_stmt->expression != NULL) ? evaluate(return_stmt->expression) : NULL_VALUE();
            ctx_return_value = return_value;
            current_context->signal = FLOW_RETURN;
            longjmp(current_context->buf, 1);
        } break;
        case AST_NODE_BREAK: {
            if (current_context == NULL || current_context->type != CTX_LOOP) {
                runtime_error("'break' is only allowed inside loops");
            }

            current_context->signal = FLOW_BREAK;
            longjmp(current_context->buf, 1);
        } break;
        case AST_NODE_CONTINUE: {
            if (current_context == NULL || current_context->type != CTX_LOOP) {
                runtime_error("'break' is only allowed inside loops");
            }

            current_context->signal = FLOW_CONTINUE;
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

            if (assignment->op == TOKEN_EQUAL) {
                *var = value;
                return *var;
            }

            if (assignment->op == TOKEN_PLUS_EQUAL && (var->type == VALUE_STRING || value.type == VALUE_STRING)) {
                if (var->type == VALUE_STRING && value.type == VALUE_STRING) {
                    var->string = string_concat(var->string, value.string);
                    return *var;
                }
                runtime_error("string concatenation is only possible for two strings");
            }

            if (var->type < VALUE_INT || var->type > VALUE_BOOL || value.type < VALUE_INT || value.type > VALUE_BOOL) {
                runtime_error(
                    "cannot perform assignment operation '%s' for '%s' and '%s'",
                    token_as_cstr(assignment->op),
                    value_type_as_cstr(var->type),
                    value_type_as_cstr(value.type)
                );
            }

            ValueType result_type = VALUE_NULL;
            if (var->type == VALUE_FLOAT || value.type == VALUE_FLOAT) result_type = VALUE_FLOAT;
            else result_type = VALUE_INT;

            *var = promote(*var, result_type);
            value = promote(value, result_type);

            switch(assignment->op) {
                case TOKEN_PLUS_EQUAL: {
                    if (result_type == VALUE_INT) var->integer += value.integer;
                    else var->floating += value.floating;
                } break;
                case TOKEN_MINUS_EQUAL: {
                    if (result_type == VALUE_INT) var->integer -= value.integer;
                    else var->floating -= value.floating;
                } break;
                case TOKEN_ASTERISK_EQUAL: {
                    if (result_type == VALUE_INT) var->integer *= value.integer;
                    else var->floating *= value.floating;
                } break;
                case TOKEN_SLASH_EQUAL: {
                    if (result_type == VALUE_INT) {
                        if (value.integer == 0) runtime_error("division by zero");
                        var->integer /= value.integer;
                    }
                    else {
                        if (value.floating == 0.0) runtime_error("division by zero");
                        var->floating /= value.floating;
                    }
                } break;
                case TOKEN_PERCENT_EQUAL: {
                    if (result_type != VALUE_INT) runtime_error("modulo operation is only allowed for integers");
                    if (value.integer == 0) runtime_error("modulo by zero");
                    var->integer %= value.integer;
                }
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

            if (binary->op == TOKEN_EQUAL_EQUAL) {
                return BOOL_VALUE(values_equal(left, right));
            }
            if (binary->op == TOKEN_NOT_EQUAL) {
                return BOOL_VALUE(!values_equal(left, right));
            }

            // ugly hack for string concatenation
            if (binary->op == TOKEN_PLUS && (left.type == VALUE_STRING || right.type == VALUE_STRING)) {
                if (left.type == VALUE_STRING && right.type == VALUE_STRING) {
                    return STRING_VALUE(string_concat(left.string, right.string));
                }
                runtime_error("string concatenation is only possible for two strings");
            }

            if (left.type < VALUE_INT || left.type > VALUE_BOOL || right.type < VALUE_INT || right.type > VALUE_BOOL) {
                runtime_error(
                    "cannot perform binary operation '%s' for '%s' and '%s'",
                    token_as_cstr(binary->op),
                    value_type_as_cstr(left.type),
                    value_type_as_cstr(right.type)
                );
            }

            ValueType result_type = VALUE_NULL;
            if (left.type == VALUE_FLOAT || right.type == VALUE_FLOAT) result_type = VALUE_FLOAT;
            else if (left.type == VALUE_INT || right.type == VALUE_INT) result_type = VALUE_INT;
            else result_type = VALUE_BOOL;

            left = promote(left, result_type);
            right = promote(right, result_type);

            switch (binary->op) {
                case TOKEN_PLUS: {
                    if (result_type == VALUE_INT) return INT_VALUE(left.integer + right.integer);
                    if (result_type == VALUE_FLOAT) return FLOAT_VALUE(left.floating + right.floating); 
                    return INT_VALUE(left.boolean + right.boolean);
                }
                case TOKEN_MINUS: {
                    if (result_type == VALUE_INT) return INT_VALUE(left.integer - right.integer);
                    if (result_type == VALUE_FLOAT) return FLOAT_VALUE(left.floating - right.floating); 
                    return INT_VALUE(left.boolean - right.boolean);
                }
                case TOKEN_ASTERISK: {
                    if (result_type == VALUE_INT) return INT_VALUE(left.integer * right.integer);
                    if (result_type == VALUE_FLOAT) return FLOAT_VALUE(left.floating * right.floating); 
                    return INT_VALUE(left.boolean * right.boolean);
                }
                case TOKEN_SLASH: {
                    if (result_type == VALUE_INT) {
                        if (right.integer == 0) runtime_error("division by zero");
                        return INT_VALUE(left.integer / right.integer);
                    }
                    if (result_type == VALUE_FLOAT) {
                        if (right.floating == 0.0) runtime_error("division by zero");
                        return FLOAT_VALUE(left.floating / right.floating);
                    }
                    if (right.boolean == false) runtime_error("division by zero");
                    return INT_VALUE(left.boolean / right.boolean);
                }
                case TOKEN_PERCENT: {
                    if (result_type != VALUE_INT) runtime_error("modulo operation is only allowed for integers");
                    if (right.integer == 0) runtime_error("modulo by zero");
                    return INT_VALUE(left.integer % right.integer);
                }
                case TOKEN_GREATER: {
                    if (result_type == VALUE_INT) return BOOL_VALUE(left.integer > right.integer);
                    if (result_type == VALUE_FLOAT) return BOOL_VALUE(left.floating > right.floating); 
                    return BOOL_VALUE(left.boolean > right.boolean);
                }
                case TOKEN_GREATER_EQUAL: {
                    if (result_type == VALUE_INT) return BOOL_VALUE(left.integer >= right.integer);
                    if (result_type == VALUE_FLOAT) return BOOL_VALUE(left.floating >= right.floating); 
                    return BOOL_VALUE(left.boolean >= right.boolean);
                }
                case TOKEN_LESS: {
                    if (result_type == VALUE_INT) return BOOL_VALUE(left.integer < right.integer);
                    if (result_type == VALUE_FLOAT) return BOOL_VALUE(left.floating < right.floating); 
                    return BOOL_VALUE(left.boolean < right.boolean);
                }
                case TOKEN_LESS_EQUAL: {
                    if (result_type == VALUE_INT) return BOOL_VALUE(left.integer <= right.integer);
                    if (result_type == VALUE_FLOAT) return BOOL_VALUE(left.floating <= right.floating); 
                    return BOOL_VALUE(left.boolean <= right.boolean);
                }
                default: break;
            }
        } break;
        case AST_NODE_UNARY: {
            ASTNodeUnary* unary = (ASTNodeUnary*)root;
            Value value = evaluate(unary->right);

            switch (unary->op) {
                case TOKEN_MINUS: {
                    if (value.type == VALUE_INT) value.integer = -value.integer;
                    else if (value.type == VALUE_FLOAT) value.floating = -value.floating;
                    else if (value.type == VALUE_BOOL) value = INT_VALUE(-value.boolean);
                    else {
                        runtime_error(
                            "cannot perform unary operation '%s' for '%s'",
                            token_as_cstr(TOKEN_MINUS),
                            value_type_as_cstr(value.type)
                        );
                    }
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
                ctx.type = CTX_FUNCTION;
                current_context = &ctx;
                Value return_value = NULL_VALUE();

                if (setjmp(ctx.buf) == 0) {
                    evaluate(callee.function->body);
                }
                else {
                    FlowSignal sig = ctx.signal;
                    if (sig == FLOW_RETURN) {
                        return_value = ctx_return_value;
                    }
                }

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
