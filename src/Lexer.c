#include "Lexer.h"
#include <stdio.h>
#include "Common.h"

#define IS_DIGIT(x) ('0' <= x && x <= '9')


static int ReadNumber(struct Lexer *lexer) {
    int result = 0;
    while (TRUE) {
        int digit = lexer->text[lexer->char_idx] - '0';
        result = result * 10 + digit;
        if (lexer->char_idx + 1 == lexer->text_size) {
            break;
        }

        if (!IS_DIGIT(lexer->text[lexer->char_idx + 1])) {
            break;
        }

        lexer->char_idx += 1;
    }

    return result;
}

void ReadToken(struct Lexer *lexer) {
    char c;
    while (TRUE) {
        if (lexer->char_idx == lexer->text_size) {
            lexer->token.line = lexer->line;
            lexer->token.type = TOKEN_EOF;
            return;
        }

        c = lexer->text[lexer->char_idx];
        if (c == '\n') {
            lexer->line += 1;
            lexer->char_idx += 1;
        }
        else if (c == ' ' || c == '\t' || c == '\r' || c == '\f') {
            lexer->char_idx += 1;
        }
        else {
            break;
        }
    }

    lexer->token.line = lexer->line;
    switch (c) {
        case '+': {
            lexer->token.type = TOKEN_PLUS;
        } break;
        case '-': {
            lexer->token.type = TOKEN_MINUS;
        } break;
        case '*': {
            lexer->token.type = TOKEN_STAR;
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

    lexer->char_idx += 1;
}