#ifndef CLITTLE_TOKEN_H
#define CLITTLE_TOKEN_H
#include "Common.h"


enum TokenType {
    // Operators
    TOKEN_EOF,
    TOKEN_PLUS, TOKEN_MINUS,
    TOKEN_STAR, // TOKEN_SLASH,
    TOKEN_TWO_EQUAL, TOKEN_EXMARK_EQUAL,
    TOKEN_LESS_THAN, TOKEN_LESS_THAN_EQUAL, TOKEN_GREATER_THAN, TOKEN_GREATER_THAN_EQUAL,
    // Keywords
    TOKEN_PRINT,
    TOKEN_INT,

    TOKEN_INVALID,
    TOKEN_EQUAL,
    TOKEN_INT_LITERAL,
    TOKEN_SEMICOLON,
    TOKEN_IDENT,
    TOKEN_LEFT_CURLY_BRAC,
    TOKEN_RIGHT_CURLY_BRAC,
};

struct Token {
    int line;
    enum TokenType type;
    union {
        int intvalue;
        char *strvalue;
    };
};

#endif // CLITTLE_TOKEN_H