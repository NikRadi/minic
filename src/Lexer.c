#include "Lexer.h"
#include "FileIO.h"
#include "ReportError.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef bool (*IsAllowedInSequenceFunction)(char *);

static bool IsAlphabetic(char c);
static bool IsDigit(char c);
static char PeekChar(struct Lexer *l);
static struct Token MakeToken(struct Lexer *l);

static void AddToken(struct Lexer *l, struct Token token) {
    token.line = l->line;
    l->tokens[l->token_index] = token;
    l->token_index = (l->token_index + 1) % LEXER_TOKEN_CACHE_SIZE;
}

static void AddTokenWithType(struct Lexer *l, enum TokenType type) {
    struct Token token = MakeToken(l);
    token.type = type;
    AddToken(l, token);
}

static void EatChar(struct Lexer *l) {
    l->code_index += 1;
}

static void EatWhitespace(struct Lexer *l) {
    bool is_done = false;
    while (!is_done) {
        switch (PeekChar(l)) {
            case '\n':
            case '\r': {
                l->line += 1;
                EatChar(l);
            } break;
            case ' ': {
                EatChar(l);
            } break;
            default: {
                is_done = true;
            } break;
        }
    }
}

static bool IsAllowedInIdentifier(char *c) {
    return IsAlphabetic(*c) || IsDigit(*c) || *c == '_';
}

static bool IsAllowedInStringLiteral(char *c) {
    return IsAlphabetic(*c) || IsDigit(*c) || *c != '"' || *c == '%';
}

static bool IsAlphabetic(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

static bool IsDigit(char c) {
    return '0' <= c && c <= '9';
}

static struct Token MakeToken(struct Lexer *l) {
    struct Token token;
    token.line = l->line;
    token.location = l->code + l->code_index;
    return token;
}

static int NumCharsLeft(struct Lexer *l) {
    return l->code_length - l->code_index;
}

static char PeekChar(struct Lexer *l) {
    return l->code[l->code_index];
}

static void ReadSequence(struct Lexer *l, char *buffer, IsAllowedInSequenceFunction IsAllowed) {
    int length = 0;
    while (NumCharsLeft(l) > 0) {
        char *c = l->code + l->code_index;
        if (!IsAllowed(c)) {
            break;
        }

        buffer[length] = *c;
        length += 1;
        EatChar(l);
    }

    buffer[length] = '\0';
}

static enum TokenType TypeOfIdentifier(char *identifier) {
    if (strcmp(identifier, "char") == 0)    return TOKEN_KEYWORD_CHAR;
    if (strcmp(identifier, "else") == 0)    return TOKEN_KEYWORD_ELSE;
    if (strcmp(identifier, "for") == 0)     return TOKEN_KEYWORD_FOR;
    if (strcmp(identifier, "int") == 0)     return TOKEN_KEYWORD_INT;
    if (strcmp(identifier, "if") == 0)      return TOKEN_KEYWORD_IF;
    if (strcmp(identifier, "return") == 0)  return TOKEN_KEYWORD_RETURN;
    if (strcmp(identifier, "sizeof") == 0)  return TOKEN_KEYWORD_SIZEOF;
    if (strcmp(identifier, "while") == 0)   return TOKEN_KEYWORD_WHILE;
    return TOKEN_IDENTIFIER;
}


//
// ===
// == Functions defined in Lexer.h
// ===
//


void Lexer_EatToken(struct Lexer *l) {
    EatWhitespace(l);
    if (NumCharsLeft(l) == 0) {
        AddTokenWithType(l, TOKEN_END_OF_FILE);
        return;
    }

    struct Token token = MakeToken(l);
    char c = PeekChar(l);
    switch (c) {
        case ',': { EatChar(l); token.type = TOKEN_COMMA; } break;
        case ';': { EatChar(l); token.type = TOKEN_SEMICOLON; } break;
        case '(': { EatChar(l); token.type = TOKEN_LEFT_ROUND_BRACKET; } break;
        case ')': { EatChar(l); token.type = TOKEN_RIGHT_ROUND_BRACKET; } break;
        case '{': { EatChar(l); token.type = TOKEN_LEFT_CURLY_BRACKET; } break;
        case '}': { EatChar(l); token.type = TOKEN_RIGHT_CURLY_BRACKET; } break;
        case '[': { EatChar(l); token.type = TOKEN_LEFT_SQUARE_BRACKET; } break;
        case ']': { EatChar(l); token.type = TOKEN_RIGHT_SQUARE_BRACKET; } break;
        case '=': {
            EatChar(l);
            switch (PeekChar(l)) {
                case '=': { EatChar(l); token.type = TOKEN_2_EQUALS; } break;
                default: { token.type = TOKEN_EQUALS; } break;
            }
        } break;
        case '*': {
            EatChar(l);
            token.type = TOKEN_STAR;
        } break;
        case '/': {
            EatChar(l);
            token.type = TOKEN_SLASH;
        } break;
        case '!': {
            EatChar(l);
            switch (PeekChar(l)) {
                case '=': { EatChar(l); token.type = TOKEN_EXCLAMATION_MARK_EQUALS; } break;
            }
        } break;
        case '%': {
            EatChar(l);
            token.type = TOKEN_PERCENTAGE;
        } break;
        case '+': {
            EatChar(l);
            token.type = TOKEN_PLUS;
        } break;
        case '&': {
            EatChar(l);
            token.type = TOKEN_AMPERSAND;
        } break;
        case '-': {
            EatChar(l);
            token.type = TOKEN_MINUS;
        } break;
        case '<': {
            EatChar(l);
            if (PeekChar(l) == '=') {
                EatChar(l);
                token.type = TOKEN_LESS_THAN_EQUALS;
            }
            else {
                token.type = TOKEN_LESS_THAN;
            }
        } break;
        case '>': {
            EatChar(l);
            if (PeekChar(l) == '=') {
                EatChar(l);
                token.type = TOKEN_GREATER_THAN_EQUALS;
            }
            else {
                token.type = TOKEN_GREATER_THAN;
            }
        } break;
        case '"': {
            EatChar(l);
            ReadSequence(l, token.str_value, IsAllowedInStringLiteral);
            token.type = TOKEN_LITERAL_STRING;
            if (PeekChar(l) != '"') {
                char *location = l->code + l->code_index;
                ReportErrorAt(l, location, "expected end of string \"");
            }

            EatChar(l);
        } break;
        default: {
            if (IsAlphabetic(c) || c == '_') {
                ReadSequence(l, token.str_value, IsAllowedInIdentifier);
                token.type = TypeOfIdentifier(token.str_value);
            }
            else if (IsDigit(c)) {
                char *p = l->code + l->code_index;
                char *q = p;
                token.int_value = strtoul(p, &p, 10);
                token.type = TOKEN_LITERAL_NUMBER;

                int len = p - q;
                l->code_index += len;
            }
            else {
                char *location = l->code + l->code_index;
                ReportErrorAt(l, location, "unknown character");
            }
        } break;
    }

    AddToken(l, token);
}

bool Lexer_Init(struct Lexer *l, char *filename) {
    struct File file;
    enum FileIOStatus status = FileIO_ReadFile(&file, filename);
    if (status == FILE_IO_ERROR_FILE_NOT_FOUND) {
        fprintf(stderr, "input file not found: %s", filename);
        return false;
    }

//    file.content = filename;
//    file.length = strlen(filename);

    l->code = file.content;
    l->code_index = 0;
    l->code_length = file.length;
    l->filename = filename;
    l->line = 1;
    l->token_index = 0;
    for (int i = 0; i < LEXER_TOKEN_CACHE_SIZE; ++i) {
        Lexer_EatToken(l);
    }

    return true;
}

struct Token Lexer_PeekToken(struct Lexer *l) {
    return Lexer_PeekToken2(l, 0);
}

struct Token Lexer_PeekToken2(struct Lexer *l, int offset) {
    int index = (l->token_index + offset) % LEXER_TOKEN_CACHE_SIZE;
    return l->tokens[index];
}
