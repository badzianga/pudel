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
    Token current;
    Token previous;
    bool had_error;
    bool panic_mode;
} Parser;

static Parser parser;

static void error_at(Token token, const char* message) {
    if (parser.panic_mode) return;
    parser.had_error = true;
    parser.panic_mode = true;
    fprintf(stderr, "[line %d] error", token.line);

    if (token.type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    }
    else if (token.type == TOKEN_ERROR) {}
    else {
        fprintf(stderr, " at '%.*s'", token.length, token.value);
    }

    fprintf(stderr, ": %s\n", message);
}

static void advance() {
    parser.previous = parser.current;

    for (;;) {
        parser.current = lexer_next_token();
        if (parser.current.type != TOKEN_ERROR) break;

        error_at(parser.current, parser.current.value);
    }
}

static bool match(int argc, ...) {
    TokenType token = parser.current.type;
    va_list argv;
    va_start(argv, argc);
    for (int i = 0; i < argc; ++i) {
        if (token == va_arg(argv, TokenType)) {
            va_end(argv);
            advance();
            return true;
        }
    }
    va_end(argv);
    return false;
}

static void consume_expected(TokenType token, const char* error_if_fail) {
    if (parser.current.type != token) {
        error_at( parser.current, error_if_fail);
        return;
    }
    advance();
}

static void synchronize() {
    parser.panic_mode = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) return;
        switch (parser.current.type) {
            case TOKEN_FOR:
            case TOKEN_FUNC:
            case TOKEN_IF:
            case TOKEN_VAR:
            case TOKEN_WHILE:
                return;
            default:
                ;
        }
        advance();
    }
}

static ASTNode* make_node_program() {
    ASTNodeBlock* node = calloc(1, sizeof(ASTNodeBlock));
    node->base.type = AST_NODE_PROGRAM;
    return (ASTNode*)node;
}

static ASTNode* make_node_block(int line) {
    ASTNodeBlock* node = calloc(1, sizeof(ASTNodeBlock));
    node->base.type = AST_NODE_BLOCK;
    node->base.line = line;
    return (ASTNode*)node;
}

static ASTNode* make_node_import(int line, String* name) {
    ASTNodeVar* node = malloc(sizeof(ASTNodeVar));
    node->base.type = AST_NODE_IMPORT;
    node->base.line = line;
    node->name = name;
    return (ASTNode*)node; 
}

static ASTNode* make_node_func_decl(int line, String* name, String** params, int param_count, ASTNode* body) {
    ASTNodeFuncDecl* node = malloc(sizeof(ASTNodeFuncDecl));
    node->base.type = AST_NODE_FUNC_DECL;
    node->base.line = line;
    node->name = name;
    node->params = params;
    node->param_count = param_count;
    node->body = body;
    return (ASTNode*)node;
}

static ASTNode* make_node_var_decl(int line, String* name, ASTNode* initializer) {
    ASTNodeVarDecl* node = malloc(sizeof(ASTNodeVarDecl));
    node->base.type = AST_NODE_VAR_DECL;
    node->base.line = line;
    node->name = name;
    node->initializer = initializer;
    return (ASTNode*)node;
}

static ASTNode* make_node_expr_stmt(int line, ASTNode* expression) {
    ASTNodeExprStmt* node = malloc(sizeof(ASTNodeExprStmt));
    node->base.type = AST_NODE_EXPR_STMT;
    node->base.line = line;
    node->expression = expression;
    return (ASTNode*)node;
}

static ASTNode* make_node_if_stmt(int line, ASTNode* condition, ASTNode* then_branch, ASTNode* else_branch) {
    ASTNodeIfStmt* node = malloc(sizeof(ASTNodeIfStmt));
    node->base.type = AST_NODE_IF_STMT;
    node->base.line = line;
    node->condition = condition;
    node->then_branch = then_branch;
    node->else_branch = else_branch;
    return (ASTNode*)node;
}

static ASTNode* make_node_while_stmt(int line, ASTNode* condition, ASTNode* body) {
    ASTNodeWhileStmt* node = malloc(sizeof(ASTNodeWhileStmt));
    node->base.type = AST_NODE_WHILE_STMT;
    node->base.line = line;
    node->condition = condition;
    node->body = body;
    return (ASTNode*)node;
}

static ASTNode* make_node_for_stmt(int line, ASTNode* initializer, ASTNode* condition, ASTNode* increment, ASTNode* body) {
    ASTNodeForStmt* node = malloc(sizeof(ASTNodeForStmt));
    node->base.type = AST_NODE_FOR_STMT;
    node->base.line = line;
    node->initializer = initializer;
    node->condition = condition;
    node->increment = increment;
    node->body = body;
    return (ASTNode*)node;
}

static ASTNode* make_node_return_stmt(int line, ASTNode* expression) {
    ASTNodeExprStmt* node = malloc(sizeof(ASTNodeExprStmt));
    node->base.type = AST_NODE_RETURN_STMT;
    node->base.line = line;
    node->expression = expression;
    return (ASTNode*)node;
}

static ASTNode* make_node_break(int line) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NODE_BREAK;
    node->line = line;
    return node;
}

static ASTNode* make_node_continue(int line) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NODE_CONTINUE;
    node->line = line;
    return node;
}

static ASTNode* make_node_assignment(int line, ASTNode* target, TokenType op, ASTNode* value) {
    ASTNodeAssignment* node = malloc(sizeof(ASTNodeAssignment));
    node->base.type = AST_NODE_ASSIGNMENT;
    node->base.line = line;
    node->target = target;
    node->op = op;
    node->value = value;
    return (ASTNode*)node;
}

static ASTNode* make_node_ternary(int line, ASTNode* condition, ASTNode* then_branch, ASTNode* else_branch) {
    ASTNodeIfStmt* node = malloc(sizeof(ASTNodeIfStmt));
    node->base.type = AST_NODE_TERNARY;
    node->base.line = line;
    node->condition = condition;
    node->then_branch = then_branch;
    node->else_branch = else_branch;
    return (ASTNode*)node;
}

static ASTNode* make_node_logical(int line, ASTNode* left, TokenType op, ASTNode* right) {
    ASTNodeBinary* node = malloc(sizeof(ASTNodeBinary));
    node->base.type = AST_NODE_LOGICAL;
    node->base.line = line;
    node->left = left;
    node->op = op;
    node->right = right;
    return (ASTNode*)node;
}

static ASTNode* make_node_binary(int line, ASTNode* left, TokenType op, ASTNode* right) {
    ASTNodeBinary* node = malloc(sizeof(ASTNodeBinary));
    node->base.type = AST_NODE_BINARY;
    node->base.line = line;
    node->left = left;
    node->op = op;
    node->right = right;
    return (ASTNode*)node;
}

static ASTNode* make_node_unary(int line, TokenType op, ASTNode* right) {
    ASTNodeUnary* node = malloc(sizeof(ASTNodeUnary));
    node->base.type = AST_NODE_UNARY;
    node->base.line = line;
    node->op = op;
    node->right = right;
    return (ASTNode*)node;
}

static ASTNode* make_node_call(int line, ASTNode* callee) {
    ASTNodeCall* node = calloc(1, sizeof(ASTNodeCall));
    node->base.type = AST_NODE_CALL;
    node->base.line = line;
    node->callee = callee;
    return (ASTNode*)node; 
}

static ASTNode* make_node_subscription(int line, ASTNode* expression, ASTNode* index) {
    ASTNodeSubscription* node = malloc(sizeof(ASTNodeSubscription));
    node->base.type = AST_NODE_SUBSCRIPTION;
    node->base.line = line;
    node->expression = expression;
    node->index = index;
    return (ASTNode*)node;
}

static ASTNode* make_node_literal(int line, Value value) {
    ASTNodeLiteral* node = malloc(sizeof(ASTNodeLiteral));
    node->base.type = AST_NODE_LITERAL;
    node->base.line = line;
    node->value = value;
    return (ASTNode*)node;
}

static ASTNode* make_node_var(int line, String* name) {
    ASTNodeVar* node = malloc(sizeof(ASTNodeVar));
    node->base.type = AST_NODE_VAR;
    node->base.line = line;
    node->name = name;
    return (ASTNode*)node;
}

static ASTNode* make_node_list(int line) {
    ASTNodeList* node = calloc(1, sizeof(ASTNodeList));
    node->base.type = AST_NODE_LIST;
    node->base.line = line;
    return (ASTNode*)node;
}

static ASTNode* parse_program();
static ASTNode* parse_global_declaration();
static ASTNode* parse_local_declaration();
static ASTNode* parse_import();
static ASTNode* parse_function_declaration();
static ASTNode* parse_variable_declaration();
static ASTNode* parse_statement();
static ASTNode* parse_expression_statement();
static ASTNode* parse_if_statement();
static ASTNode* parse_while_statement();
static ASTNode* parse_for_statement();
static ASTNode* parse_return_statement();
static ASTNode* parse_block();

static ASTNode* parse_expression();
static ASTNode* parse_assignment();
static ASTNode* parse_ternary();
static ASTNode* parse_or();
static ASTNode* parse_and();
static ASTNode* parse_equality();
static ASTNode* parse_comparison();
static ASTNode* parse_term();
static ASTNode* parse_factor();
static ASTNode* parse_unary();
static ASTNode* parse_call();
static ASTNode* parse_primary();
static ASTNode* parse_list();

static ASTNode* parse_program() {
    ASTNodeBlock* block = (ASTNodeBlock*)make_node_program();
    while (parser.current.type != TOKEN_EOF) {

        if (block->capacity < block->count + 1) {
            block->capacity = GROW_CAPACITY(block->capacity);
            block->statements = GROW_ARRAY(ASTNode*, block->statements, block->capacity);
        }
        block->statements[block->count++] = parse_global_declaration();
    }
    return (ASTNode*)block;
}

static ASTNode* parse_global_declaration() {
    ASTNode* stmt;
    if (match(1, TOKEN_VAR)) {
        stmt = parse_variable_declaration();
    }
    else if (match(1, TOKEN_FUNC)) {
        stmt = parse_function_declaration();
    }
    else if (match(1, TOKEN_IMPORT)) {
        stmt = parse_import();
    }
    else {
        stmt = parse_statement();
    }
    if (parser.had_error) synchronize();
    return stmt;
}

static ASTNode* parse_local_declaration() {
    ASTNode* stmt;
    if (match(1, TOKEN_VAR)) {
        stmt = parse_variable_declaration();
    }
    else if (match(1, TOKEN_FUNC)) {
        error_at(parser.previous, "functions can be declared only in global scope");
    }
    else if (match(1, TOKEN_IMPORT)) {
        stmt = parse_import();
    }
    else {
        stmt = parse_statement();
    }
    if (parser.had_error) synchronize();
    return stmt;
}

static ASTNode* parse_variable_declaration() {
    consume_expected(TOKEN_IDENTIFIER, "expected identifier name after declaration");
    Token identifier = parser.previous;
    String* name = string_new(identifier.value, identifier.length);

    ASTNode* initializer = NULL;
    if (match(1, TOKEN_EQUAL)) {
        initializer = parse_expression();
    }

    consume_expected(TOKEN_SEMICOLON, "expected ';' after variable declaration");
    return make_node_var_decl(identifier.line, name, initializer);
}

static ASTNode* parse_function_declaration() {
    // function name
    consume_expected(TOKEN_IDENTIFIER, "expected identifier name after declaration");
    Token identifier = parser.previous;
    String* name = string_new(identifier.value, identifier.length);

    // function parameters
    consume_expected(TOKEN_LEFT_PAREN, "expected '(' after function name");
    String** params = calloc(1, sizeof(String*));
    int param_count = 0;
    int param_capacity = 0;
    if (parser.current.type != TOKEN_RIGHT_PAREN) {
        do {
            if (param_capacity < param_count + 1) {
                param_capacity = GROW_CAPACITY(param_capacity);
                params = GROW_ARRAY(String*, params, param_capacity);
            }
            consume_expected(TOKEN_IDENTIFIER, "expected parameter name");
            String* param = string_new(parser.previous.value, parser.previous.length);
            params[param_count++] = param;
        } while (match(1, TOKEN_COMMA));
    }
    consume_expected(TOKEN_RIGHT_PAREN, "expected ')' after function parameters");

    // function body
    ASTNode* body;
    if (match(1, TOKEN_LEFT_BRACE)) {
        body = parse_block();
        consume_expected(TOKEN_RIGHT_BRACE, "expected '}' after function body");
    }
    else if (match(1, TOKEN_EQUAL)) {
        int line = parser.previous.line;
        ASTNode* return_expression = parse_expression();
        consume_expected(TOKEN_SEMICOLON, "expected ';' after function return value");
        body = make_node_return_stmt(line, return_expression);
    }
    else {
        error_at(parser.current, "expected function body");
    }

    return make_node_func_decl(identifier.line, name, params, param_count, body);
}

static ASTNode* parse_import() {
    if (match(1, TOKEN_IDENTIFIER)) {
        String* name = string_new(parser.previous.value, parser.previous.length);
        ASTNode* node = make_node_import(parser.previous.line, name);
        consume_expected(TOKEN_SEMICOLON, "expected ';' after imported module name");
        return node;
    }
    error_at(parser.current, "expected module name");
    return NULL;
}

static ASTNode* parse_statement() {
    if (match(1, TOKEN_IF)) return parse_if_statement();
    if (match(1, TOKEN_WHILE)) return parse_while_statement();
    if (match(1, TOKEN_FOR)) return parse_for_statement();
    if (match(1, TOKEN_RETURN)) return parse_return_statement();

    if (match(1, TOKEN_BREAK)) {
        int line = parser.previous.line;
        consume_expected(TOKEN_SEMICOLON, "expected ';' after 'break'");
        return make_node_break(line);
    }
    if (match(1, TOKEN_CONTINUE)) {
        int line = parser.previous.line;
        consume_expected(TOKEN_SEMICOLON, "expected ';' after 'continue'");
        return make_node_continue(line);
    }

    if (match(1, TOKEN_LEFT_BRACE)) {
        ASTNode* block = parse_block();
        consume_expected(TOKEN_RIGHT_BRACE, "expected '}' after block");
        return block;
    }

    return parse_expression_statement();
}

static ASTNode* parse_expression_statement() {
    ASTNode* expression = parse_expression();
    int line = parser.previous.line;
    consume_expected(TOKEN_SEMICOLON, "expected ';' after expression");
    return make_node_expr_stmt(line, expression);
}

static ASTNode* parse_if_statement() {
    int line = parser.previous.line;
    consume_expected(TOKEN_LEFT_PAREN, "expected '(' after 'if'");
    ASTNode* condition = parse_expression();
    consume_expected(TOKEN_RIGHT_PAREN, "expected ')' after 'if' condition");

    ASTNode* then_branch = parse_statement();
    ASTNode* else_branch = NULL;
    if (match(1, TOKEN_ELSE)) {
        else_branch = parse_statement();
    }

    return make_node_if_stmt(line, condition, then_branch, else_branch);
}

static ASTNode* parse_while_statement() {
    int line = parser.previous.line;
    consume_expected(TOKEN_LEFT_PAREN, "expected '(' after 'while'");
    ASTNode* condition = parse_expression();
    consume_expected(TOKEN_RIGHT_PAREN, "expected ')' after 'while' condition");

    if (match(1, TOKEN_SEMICOLON)) {
        return make_node_while_stmt(line, condition, NULL);
    }
    ASTNode* body = parse_statement();
    return make_node_while_stmt(line, condition, body);
}

static ASTNode* parse_for_statement() {
    int line = parser.previous.line;
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
    if (parser.current.type != TOKEN_SEMICOLON) {
        condition = parse_expression();
    }
    consume_expected(TOKEN_SEMICOLON, "expected ';' after loop condition");

    ASTNode* increment = NULL;
    if (parser.current.type != TOKEN_RIGHT_PAREN) {
        increment = parse_expression();
    }
    consume_expected(TOKEN_RIGHT_PAREN, "expected ')' after for clauses");

    ASTNode* body = parse_statement();

    if (condition == NULL) {
        condition = make_node_literal(0, BOOL_VALUE(true));
    }

    return make_node_for_stmt(line, initializer, condition, increment, body);
}

static ASTNode* parse_return_statement() {
    int line = parser.previous.line;
    ASTNode* expression = NULL;
    if (parser.current.type != TOKEN_SEMICOLON) {
        expression = parse_expression();
    }
    consume_expected(TOKEN_SEMICOLON, "expected ';' after 'return' statement");
    return make_node_return_stmt(line, expression);
}

static ASTNode* parse_block() {
    ASTNodeBlock* block = (ASTNodeBlock*)make_node_block(parser.previous.line);
    while (parser.current.type != TOKEN_RIGHT_BRACE && parser.current.type != TOKEN_EOF) {
        if (block->capacity < block->count + 1) {
            block->capacity = GROW_CAPACITY(block->capacity);
            block->statements = GROW_ARRAY(ASTNode*, block->statements, block->capacity);
        }
        block->statements[block->count++] = parse_local_declaration();
    }
    return (ASTNode*)block;
}

static ASTNode* parse_expression() {
    return parse_assignment();
}

static ASTNode* parse_assignment() {
    ASTNode* target = parse_ternary();

    if (match(6, TOKEN_EQUAL, TOKEN_PLUS_EQUAL, TOKEN_MINUS_EQUAL, TOKEN_ASTERISK_EQUAL, TOKEN_SLASH_EQUAL, TOKEN_PERCENT_EQUAL)) {
        Token op_token = parser.previous;
        ASTNode* value = parse_assignment();
        
        if (target->type == AST_NODE_VAR || target->type == AST_NODE_SUBSCRIPTION) {
            return make_node_assignment(op_token.line, target, op_token.type, value);
        }

        error_at(op_token, "invalid assignment target");
    }

    return target;
}

static ASTNode* parse_ternary() {
    ASTNode* condition = parse_or();

    if (match(1, TOKEN_QUESTION)) {
        int line = parser.previous.line;
        ASTNode* then_branch = parse_expression();
        
        consume_expected(TOKEN_COLON, "expected ':' after then branch");

        ASTNode* else_branch = parse_ternary();

        return make_node_ternary(line, condition, then_branch, else_branch);
    }
    return condition;
}

static ASTNode* parse_or() {
    ASTNode* left = parse_and();
    while (match(1, TOKEN_OR)) {
        int line = parser.previous.line;
        ASTNode* right = parse_and();
        left = make_node_logical(line, left, TOKEN_OR, right);
    }
    return left;
}

static ASTNode* parse_and() {
    ASTNode* left = parse_equality();
    while (match(1, TOKEN_AND)) {
        int line = parser.previous.line;
        ASTNode* right = parse_equality();
        left = make_node_logical(line, left, TOKEN_AND, right);
    }
    return left;
}

static ASTNode* parse_equality() {
    ASTNode* left = parse_comparison();
    while (match(2, TOKEN_EQUAL_EQUAL, TOKEN_NOT_EQUAL)) {
        Token op_token = parser.previous;
        ASTNode* right = parse_comparison();
        left = make_node_binary(op_token.line, left, op_token.type, right);
    }
    return left;
}

static ASTNode* parse_comparison() {
    ASTNode* left = parse_term();
    while (match(4, TOKEN_GREATER, TOKEN_GREATER_EQUAL, TOKEN_LESS, TOKEN_LESS_EQUAL)) {
        Token op_token = parser.previous;
        ASTNode* right = parse_term();
        left = make_node_binary(op_token.line, left, op_token.type, right);
    }
    return left;
}

static ASTNode* parse_term() {
    ASTNode* left = parse_factor();
    while(match(2, TOKEN_PLUS, TOKEN_MINUS)) {
        Token op_token = parser.previous;
        ASTNode* right = parse_factor();
        left = make_node_binary(op_token.line, left, op_token.type, right);
    }
    return left;
}

static ASTNode* parse_factor() {
    ASTNode* left = parse_unary();
    while (match(3, TOKEN_ASTERISK, TOKEN_SLASH, TOKEN_PERCENT)) {
        Token op_token = parser.previous;
        ASTNode* right = parse_unary();
        left = make_node_binary(op_token.line, left, op_token.type, right);
    }
    return left;
}

static ASTNode* parse_unary() {
    if (match(2, TOKEN_MINUS, TOKEN_NOT)) {
        Token op_token = parser.previous;
        ASTNode* right = parse_primary();
        return make_node_unary(op_token.line, op_token.type, right);
    }
    return parse_call();
}

static ASTNode* finish_call(ASTNode* callee) {
    ASTNodeCall* call = (ASTNodeCall*)make_node_call(parser.previous.line, callee);

    if (parser.current.type != TOKEN_RIGHT_PAREN) {
        do {
            if (call->capacity < call->count + 1) {
                call->capacity = GROW_CAPACITY(call->capacity);
                call->arguments = GROW_ARRAY(ASTNode*, call->arguments, call->capacity);
            }
            call->arguments[call->count++] = parse_expression();
        } while (match(1, TOKEN_COMMA));
    }

    consume_expected(TOKEN_RIGHT_PAREN, "expected ')' after arguments");

    return (ASTNode*)call;
}

static ASTNode* finish_subscription(ASTNode* expression) {
    int line = parser.previous.line;
    ASTNode* index = parse_ternary();

    consume_expected(TOKEN_RIGHT_BRACKET, "expected ']' after index");

    return make_node_subscription(line, expression, index);
}

static ASTNode* parse_call() {
    ASTNode* expr = parse_primary();
    for (;;) {
        if (match(1, TOKEN_LEFT_PAREN)) {
            expr = finish_call(expr);
        } else if (match(1, TOKEN_LEFT_BRACKET)) {
            expr = finish_subscription(expr);
        } else {
            break;
        }
    }
    return expr;
}

static ASTNode* parse_primary() {
    int line = parser.current.line;
    if (match(1, TOKEN_IDENTIFIER)) {
        String* name = string_new(parser.previous.value, parser.previous.length);
        return make_node_var(line, name);
    }
    if (match(1, TOKEN_INT)) {
        int64_t value = strtoll(parser.previous.value, NULL, 10);
        return make_node_literal(line, INT_VALUE(value));
    }
    if (match(1, TOKEN_FLOAT)) {
        int line = parser.previous.line;
        double value = strtod(parser.previous.value, NULL);
        return make_node_literal(line, FLOAT_VALUE(value));
    }
    if (match(1, TOKEN_STRING)) {
        String* string = string_new(parser.previous.value + 1, parser.previous.length - 2);
        return make_node_literal(line, STRING_VALUE(string));
    }
    if (match(1, TOKEN_TRUE)) {
        return make_node_literal(line, BOOL_VALUE(true));
    }
    if (match(1, TOKEN_FALSE)) {
        return make_node_literal(line, BOOL_VALUE(false));
    }
    if (match(1, TOKEN_NULL)) {
        return make_node_literal(line, NULL_VALUE());
    }
    if (match(1, TOKEN_LEFT_PAREN)) {
        ASTNode* inside = parse_expression();
        consume_expected(TOKEN_RIGHT_PAREN, "expected closing parenthesis");
        return inside;
    }
    if (match(1, TOKEN_LEFT_BRACKET)) {
        ASTNode* list = parse_list();
        consume_expected(TOKEN_RIGHT_BRACKET, "expected closing bracket");
        return list;
    }

    error_at(parser.current, "unexpected value");
    advance();
    return NULL;
}

static ASTNode* parse_list() {
    int line = parser.previous.line;
    ASTNodeList* list = (ASTNodeList*)make_node_list(line);

    if (parser.current.type == TOKEN_RIGHT_BRACKET) {
        return (ASTNode*)list;
    }

    do {
        if (list->capacity < list->count + 1) {
            list->capacity = GROW_CAPACITY(list->capacity);
            list->expressions = GROW_ARRAY(ASTNode*, list->expressions, list->capacity);
        }
        list->expressions[list->count++] = parse_ternary();
    } while (match(1, TOKEN_COMMA));

    return (ASTNode*)list;
}

bool parser_parse(const char* source, ASTNode** output) {
    lexer_init(source);

    parser.had_error = false;
    parser.panic_mode = false;
    advance();

    *output = parse_program();

    return !parser.had_error;
}

void parser_free_ast(ASTNode* root) {
    switch (root->type) {
        case AST_NODE_PROGRAM:
        case AST_NODE_BLOCK: {
            ASTNodeBlock* block = (ASTNodeBlock*)root;
            for (int i = 0; i < block->count; ++i) {
                parser_free_ast(block->statements[i]);
            }
            free(block->statements);
        } break;
        case AST_NODE_IMPORT: break;
        case AST_NODE_FUNC_DECL: {
            ASTNodeFuncDecl* func_decl = (ASTNodeFuncDecl*)root;
            for (int i = 0; i < func_decl->param_count; ++i) {
                free(func_decl->params[i]);
            }
            free(func_decl->params);
            parser_free_ast(func_decl->body);
        } break;
        case AST_NODE_VAR_DECL: {
            ASTNodeVarDecl* var_decl = (ASTNodeVarDecl*)root;
            parser_free_ast(var_decl->initializer);
        } break;
        case AST_NODE_EXPR_STMT: {
            ASTNodeExprStmt* expr_stmt = (ASTNodeExprStmt*)root;
            parser_free_ast(expr_stmt->expression);
        } break;
        case AST_NODE_TERNARY:
        case AST_NODE_IF_STMT: {
            ASTNodeIfStmt* if_stmt = (ASTNodeIfStmt*)root;
            parser_free_ast(if_stmt->condition);
            parser_free_ast(if_stmt->then_branch);
            if (if_stmt->else_branch != NULL) {
                parser_free_ast(if_stmt->else_branch);
            }
        } break;
        case AST_NODE_WHILE_STMT: {
            ASTNodeWhileStmt* while_stmt = (ASTNodeWhileStmt*)root;
            parser_free_ast(while_stmt->condition);
            if (while_stmt->body != NULL) {
                parser_free_ast(while_stmt->body);
            }
        } break;
        case AST_NODE_FOR_STMT: {
            ASTNodeForStmt* for_stmt = (ASTNodeForStmt*)root;
            if (for_stmt->initializer != NULL) {
                parser_free_ast(for_stmt->initializer);
            }
            if (for_stmt->increment != NULL) {
                parser_free_ast(for_stmt->increment);
            }
            if (for_stmt->body != NULL) {
                parser_free_ast(for_stmt->body);
            }
        } break;
        case AST_NODE_RETURN_STMT: {
            ASTNodeExprStmt* return_stmt = (ASTNodeExprStmt*)root;
            if (return_stmt->expression != NULL) {
                parser_free_ast(return_stmt->expression);
            }
        } break;
        case AST_NODE_BREAK: break;
        case AST_NODE_CONTINUE: break;
        case AST_NODE_ASSIGNMENT: {
            ASTNodeAssignment* assignment = (ASTNodeAssignment*)root;
            parser_free_ast(assignment->target);
            parser_free_ast(assignment->value);
        } break;
        case AST_NODE_LOGICAL:
        case AST_NODE_BINARY: {
            ASTNodeBinary* binary = (ASTNodeBinary*)root;
            parser_free_ast(binary->left);
            parser_free_ast(binary->right);
        } break;
        case AST_NODE_UNARY: {
            ASTNodeUnary* unary = (ASTNodeUnary*)root;
            parser_free_ast(unary->right);
        } break;
        case AST_NODE_CALL: {
            ASTNodeCall* call = (ASTNodeCall*)root;
            parser_free_ast(call->callee);
            for (int i = 0; i < call->count; ++i) {
                parser_free_ast(call->arguments[i]);
            }
            free(call->arguments);
        } break;
        case AST_NODE_SUBSCRIPTION: {
            ASTNodeSubscription* subscription = (ASTNodeSubscription*)root;
            parser_free_ast(subscription->expression);
            parser_free_ast(subscription->index);
        } break;
        case AST_NODE_LITERAL: break;
        case AST_NODE_VAR: break;
        case AST_NODE_LIST: {
            ASTNodeList* list = (ASTNodeList*)root;
            for (int i = 0; i < list->count; ++i) {
                parser_free_ast(list->expressions[i]);
            }
            free(list->expressions);
            // FIXME list value isn't freed
        }
    }
    free(root);
}
