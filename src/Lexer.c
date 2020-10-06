#include "Lexer.h"
#include <stdio.h>

#define IS_DIGIT(x) ('0' <= c && c <= '9')


static int ReadNumber(struct Lexer *lexer) {
    return 0;
}

void ReadToken(struct Lexer *lexer) {
    int c;
    do {
        c = fgetc(lexer->file);
        if (c == '\n') {
            lexer->line += 1;
        }
    } while (c == ' ' || c == '\t' || c == '\r' || c == '\f' || c == '\n');

    lexer->token.line = lexer->line;
    switch (c) {
        case EOF: {
            lexer->token.type = TOKEN_EOF;
        } break;
        case '+': {
            lexer->token.type = TOKEN_PLUS;
        } break;
        case '-': {
            lexer->token.type = TOKEN_MINUS;
        } break;
        default: {
            if (IS_DIGIT(c)) {
                lexer->token.intvalue = ReadNumber(lexer);
                lexer->token.type = TOKEN_INT_LITERAL;
            }
            else {
                lexer->token.type = TOKEN_INVALID;
            }
        };
    }
}