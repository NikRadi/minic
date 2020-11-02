#include "Token.h"


char *GetTokenTypeStr(TokenType type) {
    switch (type) {
#define TOKEN(name, str) case TOKEN_##name: return ##str;
#include "Token.def"
        default: {
            printf("internal error: unknown TokenType '%d'\n", type);
            exit(1);
        }
    }
}