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