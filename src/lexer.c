#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include "lexer.h"

typedef struct Lexer {
    const char* start;
    const char* current;
    int line;
} Lexer;

static Lexer lexer = { 0 };

inline static bool is_at_end() {
    return *lexer.current == '\0';
}

inline static char peek() {
    return *lexer.current;
}

inline static char peek_next() {
    return *(lexer.current + 1);
}

inline static char advance() {
    return *lexer.current++;
}

static bool advance_if(char expected) {
    if (is_at_end()) return false;
    if (*lexer.current != expected) return false;
    ++lexer.current;
    return true;
}

static Token make_token(TokenType type) {
    return (Token){
        .type = type,
        .value = lexer.start,
        .line = lexer.line,
        .length = (int)(lexer.current - lexer.start)
    };
}

static Token make_error_token(const char* message) {
    return (Token){
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
                advance();
                break;
            case '\n':
                ++lexer.line;
                advance();
                break;
            case '/':
                // simple comment - //
                if (peek_next() == '/') {
                    while (peek() != '\n' && !is_at_end()) advance();
                }
                // multi-line comment - /* */
                else if (peek_next() == '*') {
                    advance();  // consume /
                    advance();  // consume *
                    for (;;) {
                        char c = advance();
                        if (c == '\0') {
                            break; // TODO: return error token - unterminated /*
                        }
                        else if (c == '\n') {
                            ++lexer.line;
                        }
                        else if (c == '*' && peek() == '/') {
                            advance(); // consume /
                            break;
                        }
                    }
                }
                else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static Token read_number() {
    while (isdigit(peek())) advance();

    if (advance_if('.')) {
        while (isdigit(peek())) advance();

        return make_token(TOKEN_FLOAT);
    }
    return make_token(TOKEN_INT);
}

static Token read_string() {
    while (peek() != '"' && !is_at_end()) {
        if (peek() == '\n') {
            ++lexer.line;
        }
        advance();
    }

    if (is_at_end()) return make_error_token("unterminated string");

    advance();
    return make_token(TOKEN_STRING);
    
}

static TokenType check_keyword(int start, int length, const char* rest, TokenType type) {
    if (lexer.current - lexer.start == start + length && memcmp(lexer.start + start, rest, length) == 0) {
        return type;
    }
    return TOKEN_IDENTIFIER;
} 

static TokenType identifier_type() {
    switch (lexer.start[0]) {
        case 'a': return check_keyword(1, 2, "nd", TOKEN_AND);
        case 'b': return check_keyword(1, 4, "reak", TOKEN_BREAK);
        case 'c': return check_keyword(1, 7, "ontinue", TOKEN_CONTINUE);
        case 'e': return check_keyword(1, 3, "lse", TOKEN_ELSE);
        case 'f': {
            if (lexer.current - lexer.start > 1) {
                switch (lexer.start[1]) {
                    case 'a': return check_keyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o': return check_keyword(2, 1, "r", TOKEN_FOR);
                    case 'u': return check_keyword(2, 2, "nc", TOKEN_FUNC);
                    default: break;
                }
            }
        } break;
        case 'i': {
            if (lexer.current - lexer.start > 1) {
                switch (lexer.start[1]) {
                    case 'f': return (lexer.current - lexer.start == 2) ? TOKEN_IF : TOKEN_IDENTIFIER;
                    case 'm': return check_keyword(2, 4, "port", TOKEN_IMPORT);
                    default: break;
                }
            }
        } break;
        case 'n': return check_keyword(1, 3, "ull", TOKEN_NULL);
        case 'o': return check_keyword(1, 1, "r", TOKEN_OR);
        case 'r': return check_keyword(1, 5, "eturn", TOKEN_RETURN);
        case 't': return check_keyword(1, 3, "rue", TOKEN_TRUE);
        case 'v': return check_keyword(1, 2, "ar", TOKEN_VAR);
        case 'w': return check_keyword(1, 4, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

static Token read_identifier() {
    while (isalnum(peek()) || peek() == '_') {
        advance();
    }

    return make_token(identifier_type());
}

void lexer_init(const char* source) {
    lexer.start = source;
    lexer.current = source;
    lexer.line = 1;
}

Token lexer_next_token() {
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
        case '[':
            return make_token(TOKEN_LEFT_BRACKET);
        case ']':
            return make_token(TOKEN_RIGHT_BRACKET);
        case ';':
            return make_token(TOKEN_SEMICOLON);
        case ':':
            return make_token(TOKEN_COLON);
        case ',':
            return make_token(TOKEN_COMMA);
        case '?':
            return make_token(TOKEN_QUESTION);
        case '+':
            return advance_if('=') ? make_token(TOKEN_PLUS_EQUAL) : make_token(TOKEN_PLUS);
        case '-':
            return advance_if('=') ? make_token(TOKEN_MINUS_EQUAL) : make_token(TOKEN_MINUS);
        case '*':
            return advance_if('=') ? make_token(TOKEN_ASTERISK_EQUAL) : make_token(TOKEN_ASTERISK);
        case '/':
            return advance_if('=') ? make_token(TOKEN_SLASH_EQUAL) : make_token(TOKEN_SLASH);
        case '%':
            return advance_if('=') ? make_token(TOKEN_PERCENT_EQUAL) : make_token(TOKEN_PERCENT);
        case '=':
            return advance_if('=') ? make_token(TOKEN_EQUAL_EQUAL) : make_token(TOKEN_EQUAL);
        case '!':
            return advance_if('=') ? make_token(TOKEN_NOT_EQUAL) : make_token(TOKEN_NOT);
        case '>':
            return advance_if('=') ? make_token(TOKEN_GREATER_EQUAL) : make_token(TOKEN_GREATER);
        case '<':
            return advance_if('=') ? make_token(TOKEN_LESS_EQUAL) : make_token(TOKEN_LESS);
        case '"':
            return read_string();
        default:
            break;
    }

    if (isdigit(c)) return read_number();
    else if (isalpha(c) || c == '_') return read_identifier();

    return make_error_token("unexpected character");
}

const char* token_as_cstr(TokenType type) {
    static const char* token_strings[] = {
        "EOF",

        "(",
        ")",
        "{",
        "}",
        "[",
        "]",
        ";",
        ":",
        ",",
        "?",

        "+",
        "+=",
        "-",
        "-=",
        "*",
        "*=",
        "/",
        "/=",
        "\x25",
        "\x25=",
        "=",
        "==",
        "!",
        "!=",
        ">",
        ">=",
        "<",
        "<=",

        "ID",
        "INT",
        "FLOAT",
        "STRING",

        "and",
        "break",
        "continue",
        "else",
        "false",
        "for",
        "func",
        "if",
        "import",
        "null",
        "or",
        "return",
        "true",
        "var",
        "while",

        "ERROR",
    };

    static_assert(
        sizeof(token_strings) / sizeof(token_strings[0]) == TOKEN_ERROR + 1,
        "lexer::token_as_cstr: not all token types are handled"
    );

    if (type < 0 || type > TOKEN_ERROR) {
        return "UNKNOWN";
    }

    return token_strings[type];
}
