#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "memory.h"
#include "parser.h"
#include "value.h"

typedef struct Parser {
    Token* tokens;
    Token* current;
    int count;
} Parser;

typedef struct Symbol {
    const char* name;
    ValueType type;
} Symbol;

static Parser parser;
static Symbol symbols[128] = { 0 };
static int symbol_count = 0;

static ValueType get_symbol_type(const char* name) {
    const Symbol* end = symbols + symbol_count;
    for (Symbol* symbol = symbols; symbol != end; ++symbol) {
        if (strcmp(symbol->name, name) == 0) {
            return symbol->type;
        }
    }
    return VALUE_NONE;
}

static bool match(int argc, ...) {
    TokenType token = parser.current->type;
    va_list argv;
    va_start(argv, argc);
    for (int i = 0; i < argc; ++i) {
        if (token == va_arg(argv, TokenType)) {
            va_end(argv);
            ++parser.current;
            return true;
        }
    }
    va_end(argv);
    return false;
}

static void consume_expected(TokenType token, const char* error_if_fail) {
    if (parser.current->type != token) {
        fprintf(stderr, "error: %s\n", error_if_fail);
        exit(1);
    }
    ++parser.current;
}

inline static Token* previous() {
    return parser.current - 1;
}

static ASTNode* make_node_program() {
    ASTNode* node = calloc(1, sizeof(ASTNode));
    node->type = AST_NODE_PROGRAM;
    node->inferred_type = VALUE_NONE;
    return node;
}

static ASTNode* make_node_variable_declaration(char* name, ValueType type, ASTNode* initializer) {
    // TODO: check if variable already exists during parsing
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NODE_VARIABLE_DECLARATION;
    node->inferred_type = type;
    node->variable_declaration.name = name;
    node->variable_declaration.type = type;
    node->variable_declaration.initializer = initializer;

    symbols[symbol_count++] = (Symbol) { .name = name, .type = type };

    return node;
}

static ASTNode* make_node_expression_statement(ASTNode* expression) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NODE_EXPRESSION_STATEMENT;
    node->inferred_type = expression->inferred_type;
    node->expression = expression;
    return node;
}

static ASTNode* make_node_print_statement(ASTNode* expression) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NODE_PRINT_STATEMENT;
    node->inferred_type = VALUE_NONE;
    node->expression = expression;
    return node;
}

static ASTNode* make_node_if_statement(ASTNode* condition, ASTNode* then_branch, ASTNode* else_branch) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NODE_IF_STATEMENT;
    node->inferred_type = VALUE_NONE;
    node->if_statement.condition = condition;
    node->if_statement.then_branch = then_branch;
    node->if_statement.else_branch = else_branch;
    return node;
}

static ASTNode* make_node_while_statement(ASTNode* condition, ASTNode* body) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NODE_WHILE_STATEMENT;
    node->inferred_type = VALUE_NONE;
    node->while_statement.condition = condition;
    node->while_statement.body = body;
    return node;
}

static ASTNode* make_node_block() {
    ASTNode* node = calloc(1, sizeof(ASTNode));
    node->type = AST_NODE_BLOCK;
    node->inferred_type = VALUE_NONE;
    return node;
}

static ASTNode* make_node_block_filled_with(int argc, ...) {
    ASTNode* node = make_node_block();
    node->scope.capacity = argc;
    node->scope.statements = GROW_ARRAY(ASTNode*, node->scope.statements, 0, node->scope.capacity);

    va_list argv;
    va_start(argv, argc);
    for (int i = 0; i < argc; ++i) {
        node->scope.statements[node->scope.count++] = va_arg(argv, ASTNode*);
    }
    va_end(argv);
    return node;
}

static ASTNode* make_node_assignment(char* name, TokenType op, ASTNode* value) {
    // TODO: check if variable exists during parsing
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NODE_ASSIGNMENT;
    node->inferred_type = get_symbol_type(name);
    node->assignment.name = name;
    node->assignment.op = op;
    node->assignment.value = value;
    return node;
}

static ASTNode* make_node_logical(ASTNode* left, TokenType op, ASTNode* right) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NODE_LOGICAL;
    node->inferred_type = VALUE_BOOL;
    node->binary.left = left;
    node->binary.op = op;
    node->binary.right = right;
    return node;
}

static ASTNode* make_node_binary(ASTNode* left, TokenType op, ASTNode* right) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NODE_BINARY;

    ValueType lt = left->inferred_type;
    ValueType rt = right->inferred_type;

    ValueType result_type = VALUE_NONE;
    if (lt == VALUE_FLOAT || rt == VALUE_FLOAT) result_type = VALUE_FLOAT;
    else if (lt == VALUE_INT || rt == VALUE_INT) result_type = VALUE_INT;
    else result_type = VALUE_BOOL;

    node->inferred_type = result_type;
    node->binary.left = left;
    node->binary.op = op;
    node->binary.right = right;
    return node;
}

static ASTNode* make_node_unary(TokenType op, ASTNode* right) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NODE_UNARY;
    switch (op) {
        case TOKEN_MINUS: {
            node->inferred_type = (right->inferred_type == VALUE_BOOL) ? VALUE_INT : right->inferred_type;
        } break;
        case TOKEN_NOT: {
            node->inferred_type = VALUE_BOOL;
        }
        default: break;
    }
    node->unary.op = op;
    node->unary.right = right;
    return node;
}

static ASTNode* make_node_literal(Value value) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NODE_LITERAL;
    node->inferred_type = value.type;
    node->literal = value;
    return node;
}

static ASTNode* make_node_variable(char* name) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NODE_VARIABLE;
    node->inferred_type = get_symbol_type(name);
    node->name = name;
    return node;
}

static ASTNode* parse_program();
static ASTNode* parse_declaration();
static ASTNode* parse_variable_declaration();
static ASTNode* parse_statement();
static ASTNode* parse_expression_statement();
static ASTNode* parse_print_statement();
static ASTNode* parse_if_statement();
static ASTNode* parse_while_statement();
static ASTNode* parse_for_statement();
static ASTNode* parse_block();

static ASTNode* parse_expression();
static ASTNode* parse_assignment();
static ASTNode* parse_logical_or();
static ASTNode* parse_logical_and();
static ASTNode* parse_equality();
static ASTNode* parse_comparison();
static ASTNode* parse_term();
static ASTNode* parse_factor();
static ASTNode* parse_unary();
static ASTNode* parse_primary();

static ASTNode* parse_program() {
    ASTNode* node = make_node_program();
    while (parser.current->type != TOKEN_EOF) {
        if (node->scope.capacity < node->scope.count + 1) {
            int old_capacity = node->scope.capacity;
            node->scope.capacity = GROW_CAPACITY(old_capacity);
            node->scope.statements = GROW_ARRAY(ASTNode*, node->scope.statements, old_capacity, node->scope.capacity);
        }
        node->scope.statements[node->scope.count++] = parse_declaration();
    }
    return node;
}

static ASTNode* parse_declaration() {
    if (match(1, TOKEN_VAR)) {
        return parse_variable_declaration();
    }
    return parse_statement();
}

static ASTNode* parse_variable_declaration() {
    consume_expected(TOKEN_IDENTIFIER, "expected identifier name after declaration");
    char* name = strndup(previous()->value, previous()->length);

    ValueType type = VALUE_NONE;
    if (match(1, TOKEN_COLON)) {
        switch (parser.current->type) {
            case TOKEN_INT: {
                type = VALUE_INT;
            } break;
            case TOKEN_FLOAT: {
                type = VALUE_FLOAT;
            } break;
            case TOKEN_BOOL: {
                type = VALUE_BOOL;
            } break;
            default: {
                fprintf(stderr, "error: expected type after ':'\n");
                exit(1);
            } break;
        }
        ++parser.current;
    }
    else if (match(1, TOKEN_SEMICOLON)) {
        assert(false && "parser::parse_variable_declaration: type inference not implemented");
    }
    else {
        fprintf(stderr, "error: expected variable type\n");
        exit(1);
    }

    ASTNode* initializer = NULL;
    if (match(1, TOKEN_EQUAL)) {
        initializer = parse_expression();
    }

    consume_expected(TOKEN_SEMICOLON, "expected ';' after variable declaration");
    return make_node_variable_declaration(name, type, initializer);
}

static ASTNode* parse_statement() {
    if (match(1, TOKEN_PRINT)) return parse_print_statement();
    if (match(1, TOKEN_IF)) return parse_if_statement();
    if (match(1, TOKEN_WHILE)) return parse_while_statement();
    if (match(1, TOKEN_FOR)) return parse_for_statement();

    if (match(1, TOKEN_LEFT_BRACE)) {
        ASTNode* block = parse_block();
        consume_expected(TOKEN_RIGHT_BRACE, "expected '}' after block");
        return block;
    }

    return parse_expression_statement();
}

static ASTNode* parse_expression_statement() {
    ASTNode* expression = parse_expression();
    consume_expected(TOKEN_SEMICOLON, "expected ';' after expression");
    return make_node_expression_statement(expression);
}

static ASTNode* parse_print_statement() {
    ASTNode* expression = parse_expression();
    consume_expected(TOKEN_SEMICOLON, "expected ';' after expression");
    return make_node_print_statement(expression);
}

static ASTNode* parse_if_statement() {
    consume_expected(TOKEN_LEFT_PAREN, "expected '(' after 'if'");
    ASTNode* condition = parse_expression();
    consume_expected(TOKEN_RIGHT_PAREN, "expected ')' after 'if' condition");

    ASTNode* then_branch = parse_statement();
    ASTNode* else_branch = NULL;
    if (match(1, TOKEN_ELSE)) {
        else_branch = parse_statement();
    }

    return make_node_if_statement(condition, then_branch, else_branch);
}

static ASTNode* parse_while_statement() {
    consume_expected(TOKEN_LEFT_PAREN, "expected '(' after 'while'");
    ASTNode* condition = parse_expression();
    consume_expected(TOKEN_RIGHT_PAREN, "expected ')' after 'while' condition");

    if (match(1, TOKEN_SEMICOLON)) {
        return make_node_while_statement(condition, NULL);
    }
    ASTNode* body = parse_statement();
    return make_node_while_statement(condition, body);
}

static ASTNode* parse_for_statement() {
    consume_expected(TOKEN_LEFT_PAREN, "expected '(' after 'for'");

    ASTNode* initializer;
    if (match(1, TOKEN_SEMICOLON)) {
        initializer = NULL;
    }
    else if (match(1, TOKEN_VAR)) {
        initializer = parse_variable_declaration();
    }
    else {
        initializer = parse_expression_statement();
    }

    ASTNode* condition = NULL;
    if (parser.current->type != TOKEN_SEMICOLON) {
        condition = parse_expression();
    }
    consume_expected(TOKEN_SEMICOLON, "expected ';' after loop condition");

    ASTNode* increment = NULL;
    if (parser.current->type != TOKEN_RIGHT_PAREN) {
        increment = parse_expression();
    }
    consume_expected(TOKEN_RIGHT_PAREN, "expected ')' after for clauses");

    ASTNode* body = parse_statement();

    if (increment != NULL) {
        body = make_node_block_filled_with(2, body, make_node_expression_statement(increment));
    }

    if (condition == NULL) {
        condition = make_node_literal(BOOL_VALUE(true));
    }
    body = make_node_while_statement(condition, body); 

    if (initializer != NULL) {
        body = make_node_block_filled_with(2, initializer, body);
    }

    return body;
}

static ASTNode* parse_block() {
    ASTNode* node = make_node_block();
    while (parser.current->type != TOKEN_RIGHT_BRACE && parser.current->type != TOKEN_EOF) {
        if (node->scope.capacity < node->scope.count + 1) {
            int old_capacity = node->scope.capacity;
            node->scope.capacity = GROW_CAPACITY(old_capacity);
            node->scope.statements = GROW_ARRAY(ASTNode*, node->scope.statements, old_capacity, node->scope.capacity);
        }
        node->scope.statements[node->scope.count++] = parse_statement();
    }
    return node;
}

static ASTNode* parse_expression() {
    return parse_assignment();
}

static ASTNode* parse_assignment() {
    ASTNode* expression = parse_logical_or();

    if (match(6, TOKEN_EQUAL, TOKEN_PLUS_EQUAL, TOKEN_MINUS_EQUAL, TOKEN_ASTERISK_EQUAL, TOKEN_SLASH_EQUAL, TOKEN_PERCENT_EQUAL)) {
        TokenType op = previous()->type;
        ASTNode* value = parse_assignment();
        
        if (expression->type == AST_NODE_VARIABLE) {
            return make_node_assignment(expression->name, op, value);
        }

        fprintf(stderr, "error: invalid assignment target\n");
    }

    return expression;
}

static ASTNode* parse_logical_or() {
    ASTNode* left = parse_logical_and();
    while (match(1, TOKEN_LOGICAL_OR)) {
        ASTNode* right = parse_logical_and();
        left = make_node_logical(left, TOKEN_LOGICAL_OR, right);
    }
    return left;
}

static ASTNode* parse_logical_and() {
    ASTNode* left = parse_equality();
    while (match(1, TOKEN_LOGICAL_AND)) {
        ASTNode* right = parse_equality();
        left = make_node_logical(left, TOKEN_LOGICAL_AND, right);
    }
    return left;
}

static ASTNode* parse_equality() {
    ASTNode* left = parse_comparison();
    while (match(2, TOKEN_EQUAL_EQUAL, TOKEN_NOT_EQUAL)) {
        TokenType op = previous()->type;
        ASTNode* right = parse_comparison();
        left = make_node_binary(left, op, right);
    }
    return left;
}

static ASTNode* parse_comparison() {
    ASTNode* left = parse_term();
    while (match(4, TOKEN_GREATER, TOKEN_GREATER_EQUAL, TOKEN_LESS, TOKEN_LESS_EQUAL)) {
        TokenType op = previous()->type;
        ASTNode* right = parse_term();
        left = make_node_binary(left, op, right);
    }
    return left;
}

static ASTNode* parse_term() {
    ASTNode* left = parse_factor();
    while(match(2, TOKEN_PLUS, TOKEN_MINUS)) {
        TokenType op = previous()->type;
        ASTNode* right = parse_factor();
        left = make_node_binary(left, op, right);
    }
    return left;
}

static ASTNode* parse_factor() {
    ASTNode* left = parse_unary();
    while (match(3, TOKEN_ASTERISK, TOKEN_SLASH, TOKEN_PERCENT)) {
        TokenType op = previous()->type;
        ASTNode* right = parse_unary();
        left = make_node_binary(left, op, right);
    }
    return left;
}

static ASTNode* parse_unary() {
    if (match(2, TOKEN_MINUS, TOKEN_NOT)) {
        TokenType op = previous()->type;
        ASTNode* right = parse_primary();
        return make_node_unary(op, right);
    }
    return parse_primary();
}

static ASTNode* parse_primary() {
    if (match(1, TOKEN_IDENTIFIER)) {
        char* name = strndup(previous()->value, previous()->length);
        return make_node_variable(name);
    }
    if (match(1, TOKEN_INT_LITERAL)) {
        int64_t value = strtoll(previous()->value, NULL, 10);
        return make_node_literal(INT_VALUE(value));
    }
    if (match(1, TOKEN_FLOAT_LITERAL)) {
        double value = strtod(previous()->value, NULL);
        return make_node_literal(FLOAT_VALUE(value));
    }
    if (match(1, TOKEN_TRUE)) {
        return make_node_literal(BOOL_VALUE(true));
    }
    if (match(1, TOKEN_FALSE)) {
        return make_node_literal(BOOL_VALUE(false));
    }
    if (match(1, TOKEN_LEFT_PAREN)) {
        ASTNode* inside = parse_expression();
        consume_expected(TOKEN_RIGHT_PAREN, "expected closing parenthesis");
        return inside;
    }

    if (parser.current->type == TOKEN_ERROR) {
        fprintf(stderr, "error: %.*s\n", parser.current->length, parser.current->value);
    }
    else {
        fprintf(stderr, "error: unexpected value: '%.*s'\n", parser.current->length, parser.current->value);
    }
    exit(1);
}

ASTNode* parser_parse(TokenArray* token_array) {
    parser.tokens = token_array->tokens;
    parser.count = token_array->count;
    parser.current = parser.tokens;

    return parse_program();
}

void parser_free_ast(ASTNode* root) {
    switch (root->type) {
        case AST_NODE_PROGRAM: {
            for (int i = 0; i < root->scope.count; ++i) {
                free(root->scope.statements[i]);
            }
            free(root->scope.statements);
        } break;
        case AST_NODE_VARIABLE_DECLARATION: {
            free(root->variable_declaration.name);
            free(root->variable_declaration.initializer);
        } break;
        case AST_NODE_EXPRESSION_STATEMENT: {
            free(root->expression);
        } break;
        case AST_NODE_PRINT_STATEMENT: {
            free(root->expression);
        } break;
        case AST_NODE_IF_STATEMENT: {
            free(root->if_statement.condition);
            free(root->if_statement.then_branch);
            free(root->if_statement.else_branch);
        } break;
        case AST_NODE_WHILE_STATEMENT: {
            free(root->while_statement.condition);
            free(root->while_statement.body);
        } break;
        case AST_NODE_BLOCK: {
            for (int i = 0; i < root->scope.count; ++i) {
                free(root->scope.statements[i]);
            }
            free(root->scope.statements);
        } break;
        case AST_NODE_ASSIGNMENT: {
            free(root->assignment.name);
            free(root->assignment.value);
        } break;
        case AST_NODE_LOGICAL: {
            free(root->binary.left);
            free(root->binary.right);
        } break;
        case AST_NODE_BINARY: {
            free(root->binary.left);
            free(root->binary.right);
        } break;
        case AST_NODE_UNARY: {
            free(root->binary.right);
        } break;
        case AST_NODE_LITERAL: break;
        case AST_NODE_VARIABLE: {
            free(root->name);
        } break;
        default: {
            fprintf(stderr, "parser::parser_free_ast: unknown AST node type with value: %d\n", root->type);
            exit(1);
        } break;
    }
    free(root);
}
