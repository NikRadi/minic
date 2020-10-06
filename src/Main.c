#pragma warning(disable : 4996) // _CRT_SECURE_NO_WARNINGS
#include "Common.h"
#include "Lexer.h"
#include "Token.h"


int main() {
    struct Lexer lexer;
    lexer.line = 0;
    lexer.file = fopen("TestMain.c", "r");

    do {
        ReadToken(&lexer);
        printf("%d: %d\n", lexer.token.line, lexer.token.type);
    } while (lexer.token.type != TOKEN_EOF);

    return 0;
}
