#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "memory.h"
#include "parser.h"
#include "value.h"

typedef struct Parser {
    Token* tokens;
    int count;
    Token* current;
    bool had_error;
    bool panic_mode;
} Parser;

static Parser parser;

static void error_at(Token* token, const char* message) {
    if (parser.panic_mode) return;
    parser.had_error = true;
    parser.panic_mode = true;
    fprintf(stderr, "[line %d] error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    }
    else if (token->type == TOKEN_ERROR) {}
    else {
        fprintf(stderr, " at '%.*s'", token->length, token->value);
    }

    fprintf(stderr, ": %s\n", message);
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
        error_at(parser.current, error_if_fail);
        return;
    }
    ++parser.current;
}

inline static Token* previous() {
    return parser.current - 1;
}

static void synchronize() {
    parser.panic_mode = false;

    while (parser.current->type != TOKEN_EOF) {
        if (previous()->type == TOKEN_SEMICOLON) return;
        switch (parser.current->type) {
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
                return;
            default:
                ;
        }
        ++parser.current;
    }
}

static ASTNode* make_node_program() {
    ASTNodeBlock* node = calloc(1, sizeof(ASTNodeBlock));
    node->base.type = AST_NODE_PROGRAM;
    return (ASTNode*)node;
}

static ASTNode* make_node_block() {
    ASTNodeBlock* node = calloc(1, sizeof(ASTNodeBlock));
    node->base.type = AST_NODE_BLOCK;
    return (ASTNode*)node;
}

static ASTNode* make_node_block_filled_with(int argc, ...) {
    ASTNodeBlock* node = (ASTNodeBlock*)make_node_block();
    node->capacity = argc;
    node->statements = GROW_ARRAY(ASTNode*, node->statements, node->capacity);

    va_list argv;
    va_start(argv, argc);
    for (int i = 0; i < argc; ++i) {
        node->statements[node->count++] = va_arg(argv, ASTNode*);
    }
    va_end(argv);
    return (ASTNode*)node;
}

static ASTNode* make_node_var_decl(String* name, ASTNode* initializer) {
    ASTNodeVarDecl* node = malloc(sizeof(ASTNodeVarDecl));
    node->base.type = AST_NODE_VAR_DECL;
    node->name = name;
    node->initializer = initializer;
    return (ASTNode*)node;
}

static ASTNode* make_node_expr_stmt(ASTNode* expression) {
    ASTNodeExprStmt* node = malloc(sizeof(ASTNodeExprStmt));
    node->base.type = AST_NODE_EXPR_STMT;
    node->expression = expression;
    return (ASTNode*)node;
}

static ASTNode* make_node_print_stmt(ASTNode* expression) {
    ASTNodeExprStmt* node = malloc(sizeof(ASTNodeExprStmt));
    node->base.type = AST_NODE_PRINT_STMT;
    node->expression = expression;
    return (ASTNode*)node;
}

static ASTNode* make_node_if_stmt(ASTNode* condition, ASTNode* then_branch, ASTNode* else_branch) {
    ASTNodeIfStmt* node = malloc(sizeof(ASTNodeIfStmt));
    node->base.type = AST_NODE_IF_STMT;
    node->condition = condition;
    node->then_branch = then_branch;
    node->else_branch = else_branch;
    return (ASTNode*)node;
}

static ASTNode* make_node_while_stmt(ASTNode* condition, ASTNode* body) {
    ASTNodeWhileStmt* node = malloc(sizeof(ASTNodeWhileStmt));
    node->base.type = AST_NODE_WHILE_STMT;
    node->condition = condition;
    node->body = body;
    return (ASTNode*)node;
}

static ASTNode* make_node_assignment(String* name, TokenType op, ASTNode* value) {
    ASTNodeAssignment* node = malloc(sizeof(ASTNodeAssignment));
    node->base.type = AST_NODE_ASSIGNMENT;
    node->name = name;
    node->op = op;
    node->value = value;
    return (ASTNode*)node;
}

static ASTNode* make_node_logical(ASTNode* left, TokenType op, ASTNode* right) {
    ASTNodeBinary* node = malloc(sizeof(ASTNodeBinary));
    node->base.type = AST_NODE_LOGICAL;
    node->left = left;
    node->op = op;
    node->right = right;
    return (ASTNode*)node;
}

static ASTNode* make_node_binary(ASTNode* left, TokenType op, ASTNode* right) {
    ASTNodeBinary* node = malloc(sizeof(ASTNodeBinary));
    node->base.type = AST_NODE_BINARY;
    node->left = left;
    node->op = op;
    node->right = right;
    return (ASTNode*)node;
}

static ASTNode* make_node_unary(TokenType op, ASTNode* right) {
    ASTNodeUnary* node = malloc(sizeof(ASTNodeUnary));
    node->base.type = AST_NODE_UNARY;
    node->op = op;
    node->right = right;
    return (ASTNode*)node;
}

static ASTNode* make_node_literal(Value value) {
    ASTNodeLiteral* node = malloc(sizeof(ASTNodeLiteral));
    node->base.type = AST_NODE_LITERAL;
    node->value = value;
    return (ASTNode*)node;
}

static ASTNode* make_node_var(String* name) {
    ASTNodeVar* node = malloc(sizeof(ASTNodeVar));
    node->base.type = AST_NODE_VAR;
    node->name = name;
    return (ASTNode*)node;
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
static ASTNode* parse_or();
static ASTNode* parse_and();
static ASTNode* parse_equality();
static ASTNode* parse_comparison();
static ASTNode* parse_term();
static ASTNode* parse_factor();
static ASTNode* parse_unary();
static ASTNode* parse_primary();

static ASTNode* parse_program() {
    ASTNodeBlock* block = (ASTNodeBlock*)make_node_program();
    while (parser.current->type != TOKEN_EOF) {
        if (parser.panic_mode) {
            synchronize();
            continue;
        }

        if (block->capacity < block->count + 1) {
            block->capacity = GROW_CAPACITY(block->capacity);
            block->statements = GROW_ARRAY(ASTNode*, block->statements, block->capacity);
        }
        block->statements[block->count++] = parse_declaration();
    }
    return (ASTNode*)block;
}

static ASTNode* parse_declaration() {
    if (match(1, TOKEN_VAR)) {
        return parse_variable_declaration();
    }
    return parse_statement();
}

static ASTNode* parse_variable_declaration() {
    consume_expected(TOKEN_IDENTIFIER, "expected identifier name after declaration");
    int length = previous()->length;
    String* name = string_new(length);
    memcpy(name->data, previous()->value, length);

    ASTNode* initializer = NULL;
    if (match(1, TOKEN_EQUAL)) {
        initializer = parse_expression();
    }

    consume_expected(TOKEN_SEMICOLON, "expected ';' after variable declaration");
    return make_node_var_decl(name, initializer);
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
    return make_node_expr_stmt(expression);
}

static ASTNode* parse_print_statement() {
    ASTNode* expression = parse_expression();
    consume_expected(TOKEN_SEMICOLON, "expected ';' after expression");
    return make_node_print_stmt(expression);
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

    return make_node_if_stmt(condition, then_branch, else_branch);
}

static ASTNode* parse_while_statement() {
    consume_expected(TOKEN_LEFT_PAREN, "expected '(' after 'while'");
    ASTNode* condition = parse_expression();
    consume_expected(TOKEN_RIGHT_PAREN, "expected ')' after 'while' condition");

    if (match(1, TOKEN_SEMICOLON)) {
        return make_node_while_stmt(condition, NULL);
    }
    ASTNode* body = parse_statement();
    return make_node_while_stmt(condition, body);
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
        body = make_node_block_filled_with(2, body, make_node_expr_stmt(increment));
    }

    if (condition == NULL) {
        condition = make_node_literal(BOOL_VALUE(true));
    }
    body = make_node_while_stmt(condition, body); 

    if (initializer != NULL) {
        body = make_node_block_filled_with(2, initializer, body);
    }

    return body;
}

static ASTNode* parse_block() {
    ASTNodeBlock* block = (ASTNodeBlock*)make_node_block();
    while (parser.current->type != TOKEN_RIGHT_BRACE && parser.current->type != TOKEN_EOF) {
        if (block->capacity < block->count + 1) {
            block->capacity = GROW_CAPACITY(block->capacity);
            block->statements = GROW_ARRAY(ASTNode*, block->statements, block->capacity);
        }
        block->statements[block->count++] = parse_statement();
    }
    return (ASTNode*)block;
}

static ASTNode* parse_expression() {
    return parse_assignment();
}

static ASTNode* parse_assignment() {
    ASTNode* expression = parse_or();

    if (match(4, TOKEN_PLUS_EQUAL, TOKEN_MINUS_EQUAL, TOKEN_ASTERISK_EQUAL, TOKEN_SLASH_EQUAL)) {
        Token* op_token = previous();
        ASTNode* value = parse_assignment();
        
        if (expression->type == AST_NODE_VAR) {
            return make_node_assignment(((ASTNodeVar*)expression)->name, op_token->type, value);
        }

        error_at(op_token, "invalid assignment target");
    }

    return expression;
}

static ASTNode* parse_or() {
    ASTNode* left = parse_and();
    while (match(1, TOKEN_OR)) {
        ASTNode* right = parse_and();
        left = make_node_logical(left, TOKEN_OR, right);
    }
    return left;
}

static ASTNode* parse_and() {
    ASTNode* left = parse_equality();
    while (match(1, TOKEN_AND)) {
        ASTNode* right = parse_equality();
        left = make_node_logical(left, TOKEN_AND, right);
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
    if (match(1, TOKEN_IDENTIFIER)) {
        int length = previous()->length;
        String* name = string_new(length);
        memcpy(name->data, previous()->value, length);
        return make_node_var(name);
    }
    if (match(1, TOKEN_NUMBER)) {
        double value = strtod(previous()->value, NULL);
        return make_node_literal(NUMBER_VALUE(value));
    }
    if (match(1, TOKEN_STRING)) {
        int length = previous()->length - 2;
        String* string = string_new(length);
        memcpy(string->data, previous()->value + 1, length);
        return make_node_literal(STRING_VALUE(string));
    }
    if (match(1, TOKEN_TRUE)) {
        return make_node_literal(BOOL_VALUE(true));
    }
    if (match(1, TOKEN_FALSE)) {
        return make_node_literal(BOOL_VALUE(false));
    }
    if (match(1, TOKEN_NULL)) {
        return make_node_literal(NULL_VALUE());
    }
    if (match(1, TOKEN_LEFT_PAREN)) {
        ASTNode* inside = parse_expression();
        consume_expected(TOKEN_RIGHT_PAREN, "expected closing parenthesis");
        return inside;
    }

    if (parser.current->type == TOKEN_ERROR) {
        error_at(parser.current, parser.current->value);
    }
    else {
        error_at(parser.current, "unexpected value");
    }
    return NULL;
}

bool parser_parse(TokenArray* token_array, ASTNode** output) {
    parser.tokens = token_array->tokens;
    parser.count = token_array->count;
    parser.current = parser.tokens;
    parser.had_error = false;
    parser.panic_mode = false;

    *output = parse_program();

    return !parser.had_error;
}

void parser_free_ast(ASTNode* root) {
    switch (root->type) {
        case AST_NODE_PROGRAM:
        case AST_NODE_BLOCK: {
            ASTNodeBlock* block = (ASTNodeBlock*)root;
            for (int i = 0; i < block->count; ++i) {
                free(block->statements[i]);
            }
            free(block->statements);
        } break;
        case AST_NODE_VAR_DECL: {
            ASTNodeVarDecl* var_decl = (ASTNodeVarDecl*)root;
            free(var_decl->name);
            free(var_decl->initializer);
        } break;
        case AST_NODE_EXPR_STMT: {
            ASTNodeExprStmt* expr_stmt = (ASTNodeExprStmt*)root;
            free(expr_stmt->expression);
        } break;
        case AST_NODE_PRINT_STMT: {
            ASTNodeExprStmt* print_stmt = (ASTNodeExprStmt*)root;
            free(print_stmt->expression);
        } break;
        case AST_NODE_IF_STMT: {
            ASTNodeIfStmt* if_stmt = (ASTNodeIfStmt*)root;
            free(if_stmt->condition);
            free(if_stmt->then_branch);
            free(if_stmt->else_branch);
        } break;
        case AST_NODE_WHILE_STMT: {
            ASTNodeWhileStmt* while_stmt = (ASTNodeWhileStmt*)root;
            free(while_stmt->condition);
            free(while_stmt->body);
        } break;
        case AST_NODE_ASSIGNMENT: {
            ASTNodeAssignment* assignment = (ASTNodeAssignment*)root;
            free(assignment->name);
            free(assignment->value);
        } break;
        case AST_NODE_LOGICAL:
        case AST_NODE_BINARY: {
            ASTNodeBinary* binary = (ASTNodeBinary*)root;
            free(binary->left);
            free(binary->right);
        } break;
        case AST_NODE_UNARY: {
            ASTNodeUnary* unary = (ASTNodeUnary*)root;
            free(unary->right);
        } break;
        case AST_NODE_LITERAL: {
            ASTNodeLiteral* literal = (ASTNodeLiteral*)root;
            if (literal->value.type == VALUE_STRING) {
                free(literal->value.string);
            }
        } break;
        case AST_NODE_VAR: {
            ASTNodeVar* var = (ASTNodeVar*)root;
            free(var->name);
        } break;
    }
    free(root);
}
