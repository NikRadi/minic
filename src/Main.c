#include <stdio.h>
#include <stdlib.h>
#include "Common.h"
#include "Lexer.h"
#include "Parser.h"


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

    struct AstNode *ast = ParseExpr(&lexer, 0);
    return 0;
}
