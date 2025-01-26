#ifndef MINIC_LEXER_H
#define MINIC_LEXER_H
#include "List.h"
#include "Token.h"
#include <stdbool.h>

#define LEXER_TOKEN_CACHE_SIZE 2


struct Directive {
    enum TokenType type;
    char identifier[TOKEN_MAX_IDENTIFIER_LENGTH];
    // TODO: What should length of value be?
    char value[128];
};

struct Lexer {
    struct Token tokens[LEXER_TOKEN_CACHE_SIZE];
    struct List token_queue;
    struct List directives;
    char *code;
    int code_index;
    int code_length;
    int line;
    int token_index;
    int token_queue_tail;
};

void Lexer_EatToken(struct Lexer *l);

void Lexer_Init(struct Lexer *l, char *code, int code_len);

struct Token Lexer_PeekToken(struct Lexer *l);

struct Token Lexer_PeekToken2(struct Lexer *l, int offset);

#endif // MINIC_LEXER_H
