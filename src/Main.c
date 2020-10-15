#include <stdio.h>
#include <stdlib.h>
#include "Codegenx86.h"
#include "Common.h"
#include "Lexer.h"
#include "Parser.h"
#include "Typechecker.h"


int main() {
    FILE *file = fopen("TestMain.c", "rb");
    if (file == 0) {
        printf("could not read from file\n");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    Lexer lexer;
    lexer.line = 1;
    lexer.char_idx = 0;
    lexer.text_size = ftell(file);
    lexer.filename = "TestMain.c";
    rewind(file);
    lexer.text = malloc(lexer.text_size);
    fread(lexer.text, 1, lexer.text_size, file);

    ReadToken(&lexer);
    File *cfile = ParseFile(&lexer);

    FileInfo info;
    info.num_funcs = 0;
    info.num_vars = 0;
    info.filename = lexer.filename;
    info.current_func = 0;
    TypecheckFile(&info, cfile);

    info.asmfile = fopen("TestMain.asm", "w");
    if (info.asmfile == 0) {
        printf("could not write to file\n");
        return 1;
    }

    Codegenx86File(&info, cfile);
    fclose(info.asmfile);
    return 0;
}
