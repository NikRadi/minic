#ifndef CLITTLE_TOKEN_H
#define CLITTLE_TOKEN_H


enum TokenType {
    TOKEN_EOF,
    TOKEN_INVALID,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_INT_LITERAL,
};

struct Token {
    int line;
    enum TokenType type;
    int intvalue;
};

#endif // CLITTLE_TOKEN_H