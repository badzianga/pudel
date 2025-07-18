#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "lexer.h"

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

static ASTNode* parse_statement();
static ASTNode* parse_print_statement();

static ASTNode* parse_expression();
static ASTNode* parse_term();
static ASTNode* parse_factor();
static ASTNode* parse_unary();
static ASTNode* parse_primary();

static ASTNode* parse_statement() {
    if (match(1, TOKEN_PRINT)) {
        return parse_print_statement();
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

static ASTNode* parse_expression() {
    return parse_term();
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
    if (match(1, TOKEN_MINUS)) {
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

    return parse_statement();
}

void parser_free_ast(ASTNode* root) {
    switch (root->type) {
        case AST_NODE_EXPRESSION_STATEMENT: {
            free(root->expression);
        } break;
        case AST_NODE_PRINT_STATEMENT: {
            free(root->expression);
        } break;
        case AST_NODE_BINARY: {
            free(root->binary.left);
            free(root->binary.right);
        } break;
        case AST_NODE_UNARY: {
            free(root->binary.right);
        } break;
        case AST_NODE_LITERAL: break;
        default: {
            fprintf(stderr, "parser::parser_free_ast: unknown AST node type with value: %d\n", root->type);
            exit(1);
        } break;
    }
    free(root);
}
