#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "memory.h"

typedef struct Lexer {
    const char* start;
    const char* current;
    int line;
} Lexer;

static Lexer lexer;

static bool is_at_end() {
    return *lexer.current == '\0';
}

static char peek() {
    return *lexer.current;
}

static char advance() {
    if (*lexer.current == '\n') {
        lexer.line++;
    }
    return *lexer.current++;
}

static bool advance_if(char expected) {
    if (is_at_end()) return false;
    if (*lexer.current != expected) return false;
    ++lexer.current;
    return true;
}

// TODO: EOF token is not created when newline is not present at the end
static Token make_token(TokenType type) {
    return (Token) {
        .type = type,
        .value = lexer.start,
        .line = lexer.line,
        .length = (int)(lexer.current - lexer.start)
    };
}

static Token make_error_token(const char* message) {
    return (Token) {
        .type = TOKEN_ERROR,
        .value = message,
        .line = lexer.line,
        .length = strlen(message)
    };
}

static void skip_whitespace() {
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                advance();
                break;
            default:
                return;
        }
        // TODO: handle comments
    }
}

static Token read_int() {
    while (isdigit(peek())) {
        advance();
    }
    return make_token(TOKEN_INT_LITERAL);
}

static Token read_keyword() {
    while (isalpha(peek())) {
        advance();
    }

    static const char* keywords[] = {
        "print",
        "if",
        "else",
    };
    const int keywords_amount = sizeof(keywords) / sizeof(keywords[0]);

    for (int i = 0; i < keywords_amount; ++i) {
        int length = (int)(lexer.current - lexer.start);
        int keyword_length = strlen(keywords[i]);

        if (length != keyword_length) continue;

        bool might_be_keyword = true;
        for (int j = 0; j < length; ++j) {
            if (*(lexer.start + j) != keywords[i][j]) {
                might_be_keyword = false;
                break;
            }
        }
        if (might_be_keyword) {
            return make_token(TOKEN_PRINT + i);
        }
    }
    return make_error_token("identifiers are not supported yet");
}

static Token next_token() {
    skip_whitespace();
    lexer.start = lexer.current;
    
    if (is_at_end()) {
        return make_token(TOKEN_EOF);
    }

    char c = advance();
    switch (c) {
        case '(':
            return make_token(TOKEN_LEFT_PAREN);
        case ')':
            return make_token(TOKEN_RIGHT_PAREN);
        case '{':
            return make_token(TOKEN_LEFT_BRACE);
        case '}':
            return make_token(TOKEN_RIGHT_BRACE);
        case ';':
            return make_token(TOKEN_SEMICOLON);
        case '+':
            return make_token(TOKEN_PLUS);
        case '-':
            return make_token(TOKEN_MINUS);
        case '*':
            return make_token(TOKEN_ASTERISK);
        case '/':
            return make_token(TOKEN_SLASH);
        case '=':
            return advance_if('=') ? make_token(TOKEN_EQUAL_EQUAL) : make_token(TOKEN_EQUAL);
        case '!':
            return advance_if('=') ? make_token(TOKEN_NOT_EQUAL) : make_token(TOKEN_NOT);
        case '>':
            return advance_if('=') ? make_token(TOKEN_GREATER_EQUAL) : make_token(TOKEN_GREATER);
        case '<':
            return advance_if('=') ? make_token(TOKEN_LESS_EQUAL) : make_token(TOKEN_LESS);
        default:
            break;
    }
    
    if (isdigit(c)) {
        return read_int();
    }
    else {
        return read_keyword();
    }

    return make_error_token("unexpected character");
}

TokenArray lexer_lex(const char* source) {
    lexer.start = source;
    lexer.current = source;
    lexer.line = 1;

    TokenArray array = { 0 };

    while (!is_at_end()) {
        if (array.capacity < array.count + 1) {
            int old_capacity = array.capacity;
            array.capacity = GROW_CAPACITY(old_capacity);
            array.tokens = GROW_ARRAY(Token, array.tokens, old_capacity, array.capacity);
        }
        array.tokens[array.count++] = next_token();
    }
    return array;
}

void lexer_free_tokens(TokenArray* token_array) {
    token_array->tokens = reallocate(token_array->tokens, 0);
    token_array->count = 0;
    token_array->capacity = 0;
}

const char* token_as_cstr(TokenType type) {
    static const char* token_strings[] = {
        "EOF",

        "(",
        ")",
        "{",
        "}",
        ";",

        "+",
        "-",
        "*",
        "/",
        "=",
        "==",
        "!",
        "!=",
        ">",
        ">=",
        "<",
        "<=",

        "INT_LITERAL",

        "print",
        "if",
        "else",

        "ERROR",
    };

    static_assert(
        sizeof(token_strings) / sizeof(token_strings[0]) == TOKEN_ERROR + 1,
        "lexer::token_as_cstr: not all token types are handled"
    );

    if (type < 0 || type > TOKEN_ERROR) {
        fprintf(stderr, "lexer::token_as_cstr: unknown token type with value: %d\n", type);
        exit(1);
    }

    return token_strings[type];
}
