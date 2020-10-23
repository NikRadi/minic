#include <stdio.h>
#include <stdlib.h>
#include "Codegenx86.h"
#include "Common.h"
#include "Lexer.h"
#include "Parser.h"
#include "Typechecker.h"


int main(int argc, char **argv) {
    if (argc < 2) {
        printf("minic: no file name given");
        return 0;
    }

    char *filename = argv[1];
    FILE *file = fopen(filename, "rb");
    if (file == 0) {
        printf("minic: could not read from file '%s'\n", filename);
        return 1;
    }

    fseek(file, 0, SEEK_END);
    Lexer lexer;
    lexer.line = 1;
    lexer.char_idx = 0;
    lexer.text_size = ftell(file);
    lexer.filename = filename;
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

    size_t filename_len = strlen(filename);
    char *asmfilename = (char *) malloc(filename_len + 3); // len - c + asm + \0
    strncpy(asmfilename, filename, filename_len - 1);
    asmfilename[filename_len - 1] = 'a';
    asmfilename[filename_len - 0] = 's';
    asmfilename[filename_len + 1] = 'm';
    asmfilename[filename_len + 2] = '\0';
    info.asmfile = fopen(asmfilename, "w");
    if (info.asmfile == 0) {
        printf("could not write to file\n");
        return 1;
    }

    Codegenx86File(&info, cfile);
    fclose(info.asmfile);
    return 0;
}
