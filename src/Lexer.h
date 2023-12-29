#ifndef MINIC_LEXER_H
#define MINIC_LEXER_H
#include "Token.h"
#include <stdbool.h>

#define LEXER_TOKEN_CACHE_SIZE 1

struct Lexer {
    char *code;
    int code_index;
    int code_length;
    char *filename;
    int line;
    int token_index;
    struct Token tokens[LEXER_TOKEN_CACHE_SIZE];
};

void Lexer_EatToken(struct Lexer *l);

bool Lexer_Init(struct Lexer *l, char *filename);

struct Token Lexer_PeekToken(struct Lexer *l);

#endif // MINIC_LEXER_H
