#ifndef MINIC_TOKEN_H
#define MINIC_TOKEN_H
#include "Common.h"


enum TokenType {
    // Operators
    TOKEN_EOF,
    TOKEN_PLUS, TOKEN_MINUS,
    TOKEN_STAR, // TOKEN_SLASH,
    TOKEN_TWO_EQUAL, TOKEN_EXMARK_EQUAL,
    TOKEN_LESS_THAN, TOKEN_LESS_THAN_EQUAL, TOKEN_GREATER_THAN, TOKEN_GREATER_THAN_EQUAL,
    // Keywords
    TOKEN_INT,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_VOID,

    TOKEN_INVALID,
    TOKEN_INT_LITERAL,
    TOKEN_IDENT,
    TOKEN_LEFT_PAREN        = '(',
    TOKEN_RIGHT_PAREN       = ')',
    TOKEN_SEMICOLON         = ';',
    TOKEN_EQUAL             = '=',
    TOKEN_LEFT_CURLY_BRAC   = '{',
    TOKEN_RIGHT_CURLY_BRAC  = '}',

} typedef TokenType;

struct Token {
    int line;
    TokenType type;
    union {
        int intvalue;
        char *strvalue;
    };
} typedef Token;

#endif // MINIC_TOKEN_H