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
    TOKEN_SEMICOLON
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