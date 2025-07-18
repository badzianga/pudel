#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "memory.h"
#include "parser.h"

typedef struct Parser {
    Token* tokens;
    Token* current;
    int count;
} Parser;

static Parser parser;

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
    return node;
}

static ASTNode* make_node_expression_statement(ASTNode* expression) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NODE_EXPRESSION_STATEMENT;
    node->expression = expression;
    return node;
}

static ASTNode* make_node_print_statement(ASTNode* expression) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NODE_PRINT_STATEMENT;
    node->expression = expression;
    return node;
}

static ASTNode* make_node_if_statement(ASTNode* condition, ASTNode* then_branch, ASTNode* else_branch) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NODE_IF_STATEMENT;
    node->if_statement.condition = condition;
    node->if_statement.then_branch = then_branch;
    node->if_statement.else_branch = else_branch;
    return node;
}

static ASTNode* make_node_while_statement(ASTNode* condition, ASTNode* body) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NODE_WHILE_STATEMENT;
    node->while_statement.condition = condition;
    node->while_statement.body = body;
    return node;
}

static ASTNode* make_node_block() {
    ASTNode* node = calloc(1, sizeof(ASTNode));
    node->type = AST_NODE_BLOCK;
    return node;
}

static ASTNode* make_node_logical(ASTNode* left, TokenType op, ASTNode* right) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NODE_LOGICAL;
    node->binary.left = left;
    node->binary.op = op;
    node->binary.right = right;
    return node;
}

static ASTNode* make_node_binary(ASTNode* left, TokenType op, ASTNode* right) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NODE_BINARY;
    node->binary.left = left;
    node->binary.op = op;
    node->binary.right = right;
    return node;
}

static ASTNode* make_node_unary(TokenType op, ASTNode* right) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NODE_UNARY;
    node->unary.op = op;
    node->unary.right = right;
    return node;
}

static ASTNode* make_node_literal(int value) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NODE_LITERAL;
    node->literal = value;
    return node;
}

static ASTNode* make_node_variable(char* name) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NODE_VARIABLE;
    node->name = name;
    return node;
}

static ASTNode* parse_program();
static ASTNode* parse_statement();
static ASTNode* parse_print_statement();
static ASTNode* parse_if_statement();
static ASTNode* parse_while_statement();
static ASTNode* parse_block();

static ASTNode* parse_expression();
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
        node->scope.statements[node->scope.count++] = parse_statement();
    }
    return node;
}

static ASTNode* parse_statement() {
    if (match(1, TOKEN_PRINT)) return parse_print_statement();

    if (match(1, TOKEN_IF)) return parse_if_statement();

    if (match(1, TOKEN_WHILE)) return parse_while_statement();

    if (match(1, TOKEN_LEFT_BRACE)) {
        ASTNode* block = parse_block();
        consume_expected(TOKEN_RIGHT_BRACE, "expected '}' after block");
        return block;
    }

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
    return parse_logical_or();
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

    while (match(2, TOKEN_ASTERISK, TOKEN_SLASH)) {
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
    if (match(1, TOKEN_INT_LITERAL)) {
        int value = strtol(previous()->value, NULL, 10);
        return make_node_literal(value);
    }
    if (match(1, TOKEN_IDENTIFIER)) {
        char* name = strndup(previous()->value, previous()->length);
        return make_node_variable(name);
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
