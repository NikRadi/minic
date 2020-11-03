#include "Lexer.h"
#include <stdio.h>
#include "Common.h"
#include "ErrorPrint.h"

#define IS_ALPHA(x) (('a' <= x && x <= 'z') || ('A' <= x && x <= 'Z'))
#define IS_DIGIT(x) ('0' <= x && x <= '9')


static char *ReadIdent(Lexer *lexer) {
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

static int ReadNumber(Lexer *lexer) {
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

static int ReadChar(Lexer *lexer) {
    if (lexer->text[lexer->char_idx] == '\\') {
        lexer->char_idx += 1;
        switch (lexer->text[lexer->char_idx]) {
            case 'a':  return '\a';
            case 'b':  return '\b';
            case 'f':  return '\f';
            case 'n':  return '\n';
            case 'r':  return '\r';
            case 't':  return '\t';
            case 'v':  return '\v';
            case '\\': return '\\';
            case '"':  return '"' ;
            case '\'': return '\'';
            default: {
                ThrowError(lexer, "invalid escape sequence");
            }
        }
    }

    int idx = lexer->char_idx;
    lexer->char_idx += 1;
    return lexer->text[idx];
}

static char *ReadStr(Lexer *lexer) {
    int strstart = lexer->char_idx;
    int strend = strstart;
    while (lexer->text[strend] != '\"' && strend < lexer->text_size) {
        strend += 1;
    }

    lexer->char_idx = strend;
    int strlen = strend - strstart;
    char *str = (char *) malloc(strlen + 1);
    strncpy(str, lexer->text + strstart, strlen);
    str[strlen] = '\0';
    return str;
}

static void TryReadPair(Lexer *lexer, TokenType type1, char char2, TokenType type2) {
    int peek_idx = lexer->char_idx + 1;
    if (peek_idx < lexer->text_size && lexer->text[peek_idx] == char2) {
        lexer->char_idx += 1;
        lexer->peek.type = type2;
    }
    else {
        lexer->peek.type = type1;
    }
}

void ReadToken(struct Lexer *lexer) {
    lexer->token = lexer->peek;
    char c;
    while (TRUE) {
        if (lexer->char_idx == lexer->text_size) {
            lexer->peek.line = lexer->line;
            lexer->peek.type = TOKEN_EOF;
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

    lexer->peek.line = lexer->line;
    switch (c) {
        case ',':  {lexer->peek.type = TOKEN_COMMA;} break;
        case ';':  {lexer->peek.type = TOKEN_SEMICOLON;} break;
        case '{':  {lexer->peek.type = TOKEN_LEFT_CURLY_BRAC;} break;
        case '}':  {lexer->peek.type = TOKEN_RIGHT_CURLY_BRAC;} break;
        case '(':  {lexer->peek.type = TOKEN_LEFT_PAREN;} break;
        case ')':  {lexer->peek.type = TOKEN_RIGHT_PAREN;} break;
        case '[':  {lexer->peek.type = TOKEN_LEFT_SQUARE_BRAC;} break;
        case ']':  {lexer->peek.type = TOKEN_RIGHT_SQUARE_BRAC;} break;
        case '+':  {TryReadPair(lexer, TOKEN_PLUS, '=', TOKEN_PLUS_EQUAL);} break;
        case '-':  {TryReadPair(lexer, TOKEN_MINUS, '=', TOKEN_MINUS_EQUAL);} break;
        case '*':  {TryReadPair(lexer, TOKEN_STAR, '=', TOKEN_STAR_EQUAL);} break;
        case '/':  {TryReadPair(lexer, TOKEN_SLASH, '=', TOKEN_SLASH_EQUAL);} break;
        case '=':  {TryReadPair(lexer, TOKEN_EQUAL, '=', TOKEN_TWO_EQUAL);} break;
        case '!':  {TryReadPair(lexer, TOKEN_EXMARK, '=', TOKEN_EXMARK_EQUAL);} break;
        case '<':  {TryReadPair(lexer, TOKEN_LESS_THAN, '=', TOKEN_LESS_THAN_EQUAL);} break;
        case '>':  {TryReadPair(lexer, TOKEN_GREATER_THAN, '=', TOKEN_GREATER_THAN_EQUAL);} break;
        case '&':  {TryReadPair(lexer, TOKEN_AMPERSAND, '&', TOKEN_TWO_AMPERSAND);} break;
        case '|':  {TryReadPair(lexer, TOKEN_VERT_BAR, '|', TOKEN_TWO_VERT_BAR);} break;
        case '\'': {
            lexer->char_idx += 1;
            lexer->peek.intvalue = ReadChar(lexer);
            lexer->peek.type = TOKEN_LITERAL_CHAR;
            if (lexer->text[lexer->char_idx] != '\'') {
                ThrowError(lexer,
                    "%s(%d) error: char literal must end with an apostrophe",
                    lexer->filename, lexer->peek.line
                );
            }
        } break;
        case '\"': {
            lexer->char_idx += 1;
            lexer->peek.strvalue = ReadStr(lexer);
            lexer->peek.type = TOKEN_LITERAL_STR;
            if (lexer->text[lexer->char_idx] != '\"') {
                ThrowError(lexer,
                    "%s(%d) error: string literal must end with a quotation mark",
                    lexer->filename, lexer->peek.line
                );
            }
        } break;
        default: {
            if (IS_DIGIT(c)) {
                lexer->peek.intvalue = ReadNumber(lexer);
                lexer->peek.type = TOKEN_LITERAL_INT;
            }
            else if (IS_ALPHA(c) || c == '_') {
                lexer->peek.strvalue = ReadIdent(lexer);
                if (strcmp(lexer->peek.strvalue, "int") == 0) lexer->peek.type = TOKEN_INT;
                else if (strcmp(lexer->peek.strvalue, "if") == 0) lexer->peek.type = TOKEN_IF;
                else if (strcmp(lexer->peek.strvalue, "else") == 0) lexer->peek.type = TOKEN_ELSE;
                else if (strcmp(lexer->peek.strvalue, "while") == 0) lexer->peek.type = TOKEN_WHILE;
                else if (strcmp(lexer->peek.strvalue, "for") == 0) lexer->peek.type = TOKEN_FOR;
                else if (strcmp(lexer->peek.strvalue, "void") == 0) lexer->peek.type = TOKEN_VOID;
                else if (strcmp(lexer->peek.strvalue, "char") == 0) lexer->peek.type = TOKEN_CHAR;
                else if (strcmp(lexer->peek.strvalue, "return") == 0) lexer->peek.type = TOKEN_RETURN;
                else lexer->peek.type = TOKEN_IDENT;
            }
            else {
                ThrowError(lexer, "invalid character '%c'", c);
            }
        };
    }

    lexer->char_idx += 1;
}