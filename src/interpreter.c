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

static Value promote(Value value, ValueType target_type) {
    Value result = { .type = target_type };
    switch (target_type) {
        case VALUE_INT: {
            switch (value.type) {
                case VALUE_INT: return value;
                case VALUE_FLOAT: result.int_ = (int64_t) value.float_; break;
                case VALUE_BOOL: result.int_ = value.bool_ ? 1 : 0; break;
                default: {
                    fprintf(stderr, "interpreter::promote: cannot promote %d -> %d\n", value.type, target_type);
                    exit(1);
                } break;
            }
        } break;
        case VALUE_FLOAT: {
            switch (value.type) {
                case VALUE_INT: result.float_ = (double) value.int_; break;
                case VALUE_FLOAT: return value;
                case VALUE_BOOL: result.float_ = value.bool_ ? 1.0 : 0.0; break;
                default: {
                    fprintf(stderr, "interpreter::promote: cannot promote %d -> %d\n", value.type, target_type);
                    exit(1);
                } break;
            }
        } break;
        case VALUE_BOOL: {
            switch (value.type) {
                case VALUE_INT: result.bool_ = value.int_ != 0; break;
                case VALUE_FLOAT: result.bool_ = value.float_ != 0.0; break;
                case VALUE_BOOL: return value;;
                default: {
                    fprintf(stderr, "interpreter::promote: cannot promote %d -> %d\n", value.type, target_type);
                    exit(1);
                } break;
            }
        } break;
        default: {
            fprintf(stderr, "interpreter::promote: cannot promote %d -> %d\n", value.type, target_type);
            exit(1);
        } break;
    }
    return result;
}

Value interpreter_interpret(ASTNode* root) {
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
            Value value = { .type = root->variable_declaration.type , .int_ = 0 };
            if (root->variable_declaration.initializer != NULL) {
                value = promote(interpreter_interpret(root->variable_declaration.initializer), root->variable_declaration.type);
            }
            variables[variable_count++] = (Variable) { root->variable_declaration.name, value };
        } break;
        case AST_NODE_EXPRESSION_STATEMENT: {
            interpreter_interpret(root->expression);
        } break;
        case AST_NODE_PRINT_STATEMENT: {
            Value value = interpreter_interpret(root->expression);
            switch (value.type) {
                case VALUE_INT: {
                    printf("%ld\n", value.int_);
                } break;
                case VALUE_FLOAT: {
                    printf("%lf\n", value.float_);
                } break;
                case VALUE_BOOL: {
                    printf("%s\n", value.bool_ ? "true" : "false");
                } break;
                default: {
                    printf("interpreter::interpreter_interpret: invalid value type in print statement: %d\n", value.type);
                    exit(1);
                } break;
            }
        } break;
        case AST_NODE_IF_STATEMENT: {
            if (interpreter_interpret(root->if_statement.condition).int_) {
                interpreter_interpret(root->if_statement.then_branch);
            }
            else if (root->if_statement.else_branch != NULL) {
                interpreter_interpret(root->if_statement.else_branch);
            }
        } break;
        case AST_NODE_WHILE_STATEMENT: {
            while (interpreter_interpret(root->while_statement.condition).int_) {
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
            Value value = promote(interpreter_interpret(root->assignment.value), var->value.type);
            switch(root->assignment.op) {
                case TOKEN_PLUS_EQUAL: {
                    if (var->value.type == VALUE_INT) var->value.int_ += value.int_;
                    else if (var->value.type == VALUE_FLOAT) var->value.float_ += value.float_;
                } break;
                case TOKEN_MINUS_EQUAL: {
                    if (var->value.type == VALUE_INT) var->value.int_ -= value.int_;
                    else if (var->value.type == VALUE_FLOAT) var->value.float_ -= value.float_;
                    else var->value.bool_ -= value.bool_;
                } break;
                case TOKEN_ASTERISK_EQUAL: {
                    if (var->value.type == VALUE_INT) var->value.int_ *= value.int_;
                    else if (var->value.type == VALUE_FLOAT) var->value.float_ *= value.float_;
                    else var->value.bool_ *= value.bool_;
                } break;
                case TOKEN_SLASH_EQUAL: {
                    if (var->value.type == VALUE_INT) {
                        if (value.int_ == 0) {
                            fprintf(stderr, "error: division by zero\n");
                            exit(1);
                        }
                        var->value.int_ /= value.int_;
                    }
                    else if (var->value.type == VALUE_FLOAT) {
                        if (value.float_ == 0) {
                            fprintf(stderr, "error: division by zero\n");
                            exit(1);
                        }
                        var->value.float_ /= value.float_;
                    }
                    else {
                        if (value.bool_ == 0) {
                            fprintf(stderr, "error: division by zero\n");
                            exit(1);
                        }
                        var->value.bool_ /= value.bool_;
                    }
                } break;
                case TOKEN_PERCENT_EQUAL: {
                    if (var->value.type == VALUE_INT) {
                        if (value.int_ == 0) {
                            fprintf(stderr, "error: remainder by zero is undefined\n");
                            exit(1);
                        }
                        var->value.int_ %= value.int_;
                    }
                    if (var->value.type == VALUE_FLOAT) {
                        fprintf(stderr, "error: modulo operation for float is undefined\n");
                        exit(1);
                    }
                    if (value.bool_ == 0) {  // must be result_type == VALUE_BOOL
                        fprintf(stderr, "error: remainder by zero is undefined\n");
                        exit(1);
                    }
                    var->value.bool_ %= value.bool_;
                } break;
                case TOKEN_EQUAL: {
                    if (var->value.type == VALUE_INT) var->value.int_ = value.int_;
                    else if (var->value.type == VALUE_FLOAT) var->value.float_ = value.float_;
                    else var->value.bool_ = value.bool_;
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
            Value left = promote(interpreter_interpret(root->binary.left), VALUE_BOOL);
            switch (root->binary.op) {
                case TOKEN_LOGICAL_OR: {
                    if (left.bool_) return left;
                } break;
                case TOKEN_LOGICAL_AND: {
                    if (!left.bool_) return left;
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
            return promote(interpreter_interpret(root->binary.right), VALUE_BOOL);
        } break;
        case AST_NODE_BINARY: {
            Value left = interpreter_interpret(root->binary.left);
            Value right = interpreter_interpret(root->binary.right);

            // TODO: I think I should use root->inferred_type here, but I'm not sure
            ValueType result_type = VALUE_NONE;
            if (left.type == VALUE_FLOAT || right.type == VALUE_FLOAT) result_type = VALUE_FLOAT;
            else if (left.type == VALUE_INT || right.type == VALUE_INT) result_type = VALUE_INT;
            else result_type = VALUE_BOOL;

            left = promote(left, result_type);
            right = promote(right, result_type);

            switch (root->binary.op) {
                case TOKEN_PLUS:
                    if (result_type == VALUE_INT) return INT_VALUE(left.int_ + right.int_);
                    if (result_type == VALUE_FLOAT) return FLOAT_VALUE(left.float_ + right.float_);
                    return INT_VALUE(left.bool_ + right.bool_);
                case TOKEN_MINUS:
                    if (result_type == VALUE_INT) return INT_VALUE(left.int_ - right.int_);
                    if (result_type == VALUE_FLOAT) return FLOAT_VALUE(left.float_ - right.float_);
                    return INT_VALUE(left.bool_ - right.bool_);
                case TOKEN_ASTERISK:
                    if (result_type == VALUE_INT) return INT_VALUE(left.int_ * right.int_);
                    if (result_type == VALUE_FLOAT) return FLOAT_VALUE(left.float_ * right.float_);
                    return INT_VALUE(left.bool_ * right.bool_);
                case TOKEN_SLASH: {
                    if (right.bool_ == 0 || right.int_ == 0) {
                        fprintf(stderr, "error: division by zero\n");
                        exit(1);
                    }
                    if (result_type == VALUE_INT) return INT_VALUE(left.int_ / right.int_);
                    if (result_type == VALUE_FLOAT) return FLOAT_VALUE(left.float_ / right.float_);
                    return INT_VALUE(left.bool_ / right.bool_);
                }
                case TOKEN_PERCENT: {
                    if (result_type == VALUE_INT) {
                        if (right.int_ == 0) {
                            fprintf(stderr, "error: remainder by zero is undefined\n");
                            exit(1);
                        }
                        return INT_VALUE(left.int_ % right.int_);
                    }
                    if (result_type == VALUE_FLOAT) {
                        fprintf(stderr, "error: modulo operation for float is undefined\n");
                        exit(1);
                    }
                    if (right.bool_ == 0) {  // must be result_type == VALUE_BOOL
                        fprintf(stderr, "error: remainder by zero is undefined\n");
                        exit(1);
                    }
                    return INT_VALUE(left.bool_ % right.bool_);
                }
                case TOKEN_EQUAL_EQUAL:
                    if (result_type == VALUE_INT) return BOOL_VALUE(left.int_ == right.int_);
                    if (result_type == VALUE_FLOAT) return BOOL_VALUE(left.float_ == right.float_);
                    return BOOL_VALUE(left.bool_ == right.bool_);
                case TOKEN_NOT_EQUAL:
                    if (result_type == VALUE_INT) return BOOL_VALUE(left.int_ != right.int_);
                    if (result_type == VALUE_FLOAT) return BOOL_VALUE(left.float_ != right.float_);
                    return BOOL_VALUE(left.bool_ != right.bool_);
                case TOKEN_GREATER:
                    if (result_type == VALUE_INT) return BOOL_VALUE(left.int_ > right.int_);
                    if (result_type == VALUE_FLOAT) return BOOL_VALUE(left.float_ > right.float_);
                    return BOOL_VALUE(left.bool_ > right.bool_);
                case TOKEN_GREATER_EQUAL:
                    if (result_type == VALUE_INT) return BOOL_VALUE(left.int_ >= right.int_);
                    if (result_type == VALUE_FLOAT) return BOOL_VALUE(left.float_ >= right.float_);
                    return BOOL_VALUE(left.bool_ >= right.bool_);
                case TOKEN_LESS:
                    if (result_type == VALUE_INT) return BOOL_VALUE(left.int_ < right.int_);
                    if (result_type == VALUE_FLOAT) return BOOL_VALUE(left.float_ < right.float_);
                    return BOOL_VALUE(left.bool_ < right.bool_);
                case TOKEN_LESS_EQUAL:
                    if (result_type == VALUE_INT) return BOOL_VALUE(left.int_ <= right.int_);
                    if (result_type == VALUE_FLOAT) return BOOL_VALUE(left.float_ <= right.float_);
                    return BOOL_VALUE(left.bool_ <= right.bool_);
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

            switch (root->unary.op) {
                case TOKEN_MINUS: {
                    switch (value.type) {
                        case VALUE_INT: {
                            value.int_ = -value.int_;
                        } break;
                        case VALUE_FLOAT: {
                            value.float_ = -value.float_;
                        } break;
                        case VALUE_BOOL: {
                            value = promote(value, VALUE_INT);
                            value.int_ = -value.int_;
                        } break;
                        default: {
                            fprintf(stderr, "interpreter::interpreter_interpret: invalid value type in unary '-': %d\n", value.type);
                            exit(1);
                        } break;
                    }
                    return value;
                } break;
                case TOKEN_NOT: {
                    value = promote(value, VALUE_BOOL);
                    value.bool_ = !value.bool_;
                    return value;
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
