#ifndef MINIC_TOKEN_H
#define MINIC_TOKEN_H
#include "Common.h"


enum TokenType {
#define TOKEN(name, str) TOKEN_##name,
#include "TokenTypes.def"
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