#ifndef MINIC_LEXER_H
#define MINIC_LEXER_H
#include "Token.h"


struct Lexer {
    int line;
    int char_idx;
    char *filename;
    int text_size;
    char *text;
    Token token;
    Token peek;
} typedef Lexer;

void ReadToken(Lexer *lexer);

#endif // MINIC_LEXER_H