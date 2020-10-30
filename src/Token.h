#ifndef MINIC_TOKEN_H
#define MINIC_TOKEN_H
#include "Common.h"


enum TokenType {
#define TOKEN(name, str) TOKEN_##name,
#include "Token.def"
} typedef TokenType;

struct Token {
    int line;
    TokenType type;
    union {
        int intvalue;
        char *strvalue;
    };
} typedef Token;


char *GetTokenTypeStr(TokenType type);

#endif // MINIC_TOKEN_H