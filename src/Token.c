#include "Token.h"


char *GetTokenTypeStr(TokenType type) {
    switch (type) {
        default: {
            printf("internal error: unknown TokenType '%d'\n", type);
            exit(1);
        }
#define TOKEN(name, str) case TOKEN_##name: return ##str;
#include "Token.def"
    }
}