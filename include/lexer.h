#pragma once

typedef enum TokenType {
    TOKEN_EOF,            // EOF

    TOKEN_LEFT_PAREN,     // (
    TOKEN_RIGHT_PAREN,    // )
    TOKEN_LEFT_BRACE,     // {
    TOKEN_RIGHT_BRACE,    // }
    TOKEN_SEMICOLON,      // ;

    TOKEN_PLUS,           // +
    TOKEN_MINUS,          // -
    TOKEN_ASTERISK,       // *
    TOKEN_SLASH,          // /
    TOKEN_EQUAL,          // =
    TOKEN_EQUAL_EQUAL,    // ==
    TOKEN_NOT,            // !
    TOKEN_NOT_EQUAL,      // !=
    TOKEN_GREATER,        // >
    TOKEN_GREATER_EQUAL,  // >=
    TOKEN_LESS,           // <
    TOKEN_LESS_EQUAL,     // <=
    TOKEN_BIT_AND,        // &
    TOKEN_LOGICAL_AND,    // &&
    TOKEN_BIT_OR,         // |
    TOKEN_LOGICAL_OR,     // ||

    TOKEN_INT_LITERAL,    // 0

    TOKEN_PRINT,          // print
    TOKEN_IF,             // if
    TOKEN_ELSE,           // else
    TOKEN_WHILE,          // while
    TOKEN_FOR,            // for

    TOKEN_ERROR,          // ERROR
} TokenType;

typedef struct Token {
    TokenType type;
    const char* value;
    int line;
    int length;
} Token;

typedef struct TokenArray {
    Token* tokens;
    int count;
    int capacity;
} TokenArray;

TokenArray lexer_lex(const char* source);
void lexer_free_tokens(TokenArray* token_array);

const char* token_as_cstr(TokenType type);
