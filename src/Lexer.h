#ifndef CLITTLE_LEXER_H
#define CLITTLE_LEXER_H
#include "Token.h"

#include <stdio.h>


struct Lexer {
    int line;
    FILE *file;
    struct Token token;
};

void ReadToken(struct Lexer *lexer);

#endif // CLITTLE_LEXER_H