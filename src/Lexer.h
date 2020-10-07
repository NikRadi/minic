#ifndef CLITTLE_LEXER_H
#define CLITTLE_LEXER_H
#include "Token.h"


struct Lexer {
    int line;
    int char_idx;
    int text_size;
    char *text;
    struct Token token;
    struct Token peek;
};

void ReadToken(struct Lexer *lexer);

#endif // CLITTLE_LEXER_H