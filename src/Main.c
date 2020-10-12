#include <stdio.h>
#include <stdlib.h>
#include "Codegenx86.h"
#include "Common.h"
#include "Lexer.h"
#include "Parser.h"
#include "Typechecking.h"


int main() {
    FILE *file = fopen("TestMain.c", "rb");
    if (file == 0) {
        printf("could not read from file\n");
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

    ReadToken(&lexer);
    struct AstNode *ast = ParseFile(&lexer);
    TypecheckFile(ast);
    file = fopen("TestMain.asm", "w");
    if (file == 0) {
        printf("could not write to file\n");
        return 1;
    }

    Codegenx86File(file, ast);
    fclose(file);
    return 0;
}
