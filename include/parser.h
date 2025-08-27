#pragma once
#include "lexer.h"
#include "value.h"

typedef enum {
    AST_NODE_PROGRAM,
    AST_NODE_BLOCK,
    AST_NODE_FUNC_DECL,
    AST_NODE_VAR_DECL,
    AST_NODE_EXPR_STMT,
    AST_NODE_IF_STMT,
    AST_NODE_WHILE_STMT,
    AST_NODE_FOR_STMT,
    AST_NODE_RETURN_STMT,
    AST_NODE_BREAK,
    AST_NODE_CONTINUE,

    AST_NODE_ASSIGNMENT,
    AST_NODE_TERNARY,
    AST_NODE_LOGICAL,
    AST_NODE_BINARY,
    AST_NODE_UNARY,
    AST_NODE_CALL,
    AST_NODE_SUBSCRIPTION,
    AST_NODE_LITERAL,
    AST_NODE_LIST,
    AST_NODE_VAR,
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
} ASTNode;

typedef struct {
    ASTNode base;

    ASTNode** statements;
    int count;
    int capacity;
} ASTNodeBlock;

typedef struct {
    ASTNode base;

    String* name;
    String** params;
    int param_count;
    ASTNode* body;
} ASTNodeFuncDecl;

typedef struct {
    ASTNode base;

    String* name;
    ASTNode* initializer;
} ASTNodeVarDecl;

typedef struct {
    ASTNode base;

    ASTNode* expression;
} ASTNodeExprStmt;

typedef struct {
    ASTNode base;

    ASTNode* condition;
    ASTNode* then_branch;
    ASTNode* else_branch;
} ASTNodeIfStmt;

typedef struct {
    ASTNode base;

    ASTNode* condition;
    ASTNode* body;
} ASTNodeWhileStmt;

typedef struct {
    ASTNode base;

    ASTNode* initializer;
    ASTNode* condition;
    ASTNode* increment;
    ASTNode* body;
} ASTNodeForStmt;

typedef struct {
    ASTNode base;

    TokenType op;
    ASTNode* target;
    ASTNode* value;
} ASTNodeAssignment;

typedef struct {
    ASTNode base;

    TokenType op;
    ASTNode* left;
    ASTNode* right;
} ASTNodeBinary;

typedef struct {
    ASTNode base;

    TokenType op;
    ASTNode* right;
} ASTNodeUnary;

typedef struct {
    ASTNode base;

    ASTNode* callee;
    ASTNode** arguments;
    int count;
    int capacity;
} ASTNodeCall;

typedef struct {
    ASTNode base;

    ASTNode* expression;
    ASTNode* index;
} ASTNodeSubscription;

typedef struct {
    ASTNode base;

    Value value;
} ASTNodeLiteral;

typedef struct {
    ASTNode base;

    String* name;
} ASTNodeVar;

typedef struct {
    ASTNode base;

    ASTNode** expressions;
    int count;
    int capacity;
} ASTNodeList;

bool parser_parse(const char* source, ASTNode** output);
void parser_free_ast(ASTNode* root);
