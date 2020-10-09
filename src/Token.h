#ifndef CLITTLE_TOKEN_H
#define CLITTLE_TOKEN_H
#include "Common.h"


enum TokenType {
    TOKEN_EOF,
    TOKEN_INVALID,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_INT_LITERAL,
    TOKEN_PRINT,
    TOKEN_SEMICOLON,
    TOKEN_EQUAL,
    TOKEN_INT,
    TOKEN_IDENT,
    TOKEN_LEFT_CURLY_BRAC,
    TOKEN_RIGHT_CURLY_BRAC
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