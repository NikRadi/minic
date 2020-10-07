#include "Lexer.h"
#include <stdio.h>
#include "Common.h"

#define IS_ALPHA(x) (('a' <= x && x <= 'z') || ('A' <= x && x <= 'Z'))
#define IS_DIGIT(x) ('0' <= x && x <= '9')


static char *ReadIdent(struct Lexer *lexer) {
    int ident_start = lexer->char_idx;
    int ident_len = 1;
    while (TRUE) {
        if (lexer->char_idx + 1 == lexer->text_size) {
            break;
        }

        char c = lexer->text[lexer->char_idx + 1];
        if (!IS_ALPHA(c) && !IS_DIGIT(c) && c != '_') {
            break;
        }

        ident_len += 1;
        lexer->char_idx += 1;
    }

    char *result = (char *) malloc(ident_len + 1);
    result[ident_len] = 0;
    strncpy(result, &lexer->text[ident_start], ident_len);
    return result;
}

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
        case ';': {
            lexer->token.type = TOKEN_SEMICOLON;
        } break;
        default: {
            if (IS_DIGIT(c)) {
                lexer->token.intvalue = ReadNumber(lexer);
                lexer->token.type = TOKEN_INT_LITERAL;
            }
            else if (IS_ALPHA(c) || c == '_') {
                lexer->token.strvalue = ReadIdent(lexer);
                if (strcmp(lexer->token.strvalue, "print") == 0) {
                    lexer->token.type = TOKEN_PRINT;
                }
                else {
                    printf("invalid identifier\n");
                    exit(1);
                }
            }
            else {
                lexer->token.type = TOKEN_INVALID;
            }
        };
    }

    lexer->char_idx += 1;
}