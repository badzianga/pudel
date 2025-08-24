#pragma once

typedef enum TokenType {
    TOKEN_EOF,             // EOF

    TOKEN_LEFT_PAREN,      // (
    TOKEN_RIGHT_PAREN,     // )
    TOKEN_LEFT_BRACE,      // {
    TOKEN_RIGHT_BRACE,     // }
    TOKEN_LEFT_BRACKET,    // [
    TOKEN_RIGHT_BRACKET,   // ]
    TOKEN_SEMICOLON,       // ;
    TOKEN_COLON,           // :
    TOKEN_COMMA,           // ,
    TOKEN_QUESTION,        // ?

    TOKEN_PLUS,            // +
    TOKEN_PLUS_EQUAL,      // +=
    TOKEN_MINUS,           // -
    TOKEN_MINUS_EQUAL,     // -=
    TOKEN_ASTERISK,        // *
    TOKEN_ASTERISK_EQUAL,  // *=
    TOKEN_SLASH,           // /
    TOKEN_SLASH_EQUAL,     // /=
    TOKEN_PERCENT,         // %
    TOKEN_PERCENT_EQUAL,   // %=
    TOKEN_EQUAL,           // =
    TOKEN_EQUAL_EQUAL,     // ==
    TOKEN_NOT,             // !
    TOKEN_NOT_EQUAL,       // !=
    TOKEN_GREATER,         // >
    TOKEN_GREATER_EQUAL,   // >=
    TOKEN_LESS,            // <
    TOKEN_LESS_EQUAL,      // <=

    TOKEN_IDENTIFIER,      // x
    TOKEN_INT,             // 0
    TOKEN_FLOAT,           // 0.0
    TOKEN_STRING,          // ""

    TOKEN_AND,             // and
    TOKEN_BREAK,           // break
    TOKEN_CONTINUE,        // continue
    TOKEN_ELSE,            // else
    TOKEN_FALSE,           // false
    TOKEN_FOR,             // for
    TOKEN_FUNC,            // func
    TOKEN_IF,              // if
    TOKEN_NULL,            // null
    TOKEN_OR,              // or
    TOKEN_RETURN,          // return
    TOKEN_TRUE,            // true
    TOKEN_VAR,             // var
    TOKEN_WHILE,           // while

    TOKEN_ERROR,           // ERROR
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
