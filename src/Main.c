#pragma warning(disable : 4996) // _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include "Lexer.h"


int main() {
    FILE *file = fopen("TestMain.c", "rb");
    if (file == 0) {
        printf("could not open file\n");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    struct Lexer lexer;
    lexer.line = 1;
    lexer.char_idx = 0;
    lexer.text_size = ftell(file);
    rewind(file);
    lexer.text = malloc(lexer.text_size);
    fread(lexer.text, 1, lexer.text_size, file);
    printf("'%s'\n", lexer.text);

    do {
        ReadToken(&lexer);
        printf("%d, %d, %d\n",
            lexer.token.line,
            lexer.token.type,
            lexer.token.intvalue
        );
    } while (lexer.token.type != TOKEN_EOF);

    return 0;
}
