#include "AstNodes.h"


char *GetAstTypeStr(AstType type) {
    switch (type) {
#define AST(name, str) case AST_##name: return ##str;
#include "AstNodes.def"
        default: {
            printf("internal error: unknown AstType: '%d'\n", type);
            exit(1);
        }
    }
}