#include "Token.h"
#include "ErrorPrint.h"


char *GetTokenTypeStr(TokenType type) {
    switch (type) {
#define TOKEN(name, str) case TOKEN_##name: return ##str;
#include "Token.def"
        default: {
            ThrowInternalError("unknown TokenType '%d'", type);
            return 0; // To get rid of warning C4701
        }
    }
}

void PrintTokenStr(Token token) {
    switch (token.type) {
#define TOKEN(name, str) case TOKEN_##name: {printf("<Token type=\"%s\"/>", #name);} break;
#include "Token.def"
        default: {
            ThrowInternalError("unknown TokenType '%d'", token.type);
        }
    }
}