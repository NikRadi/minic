#include "AstNodes.h"
#include "ErrorPrint.h"


char *GetAstTypeStr(AstType type) {
    switch (type) {
#define AST(name, str) case AST_##name: return ##str;
#include "AstNodes.def"
        default: {
            ThrowInternalError("unknown AstType '%d'", type);
            return 0; // To get rid of warning C4701
        }
    }
}