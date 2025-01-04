#include "CodeGeneratorX86.h"
#include "Lexer.h"
#include "Parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILENAME_LENGTH 128

void ChangeFileExtension( char *filename, char *new_filename, char *new_extension) {
    char *dot = strrchr(filename, '.');
    if (dot != NULL) {
        size_t dot_position = dot - filename;
        strncpy(new_filename, filename, dot_position);
        new_filename[dot_position] = '\0';
    }
    else {
        strcpy(new_filename, filename);
    }

    strcat(new_filename, ".");
    strcat(new_filename, new_extension);
}

int main(int num_args, char **args) {
    if (num_args < 2) {
        fprintf(stderr, "error: no input file specified\n");
        return 1;
    }

    char *filename = args[1];
    struct Lexer lexer;
    if (!Lexer_Init(&lexer, filename)) {
        return 1;
    }

    struct TranslationUnit *t_unit = Parser_MakeAst(&lexer);

    filename = "tmp";
    char asm_filename[MAX_FILENAME_LENGTH];
    ChangeFileExtension(filename, asm_filename, "asm");
    FILE *asm_file;
    fopen_s(&asm_file, asm_filename, "w");

    CodeGeneratorX86_GenerateCode(asm_file, t_unit);
    fclose(asm_file);

    char obj_filename[MAX_FILENAME_LENGTH];
    ChangeFileExtension(filename, obj_filename, "obj");

    char command[256];
    sprintf(command, "nasm -f win64 %s -o %s", asm_filename, obj_filename);
    printf("%s\n", command);
    system(command);

    sprintf(command, "link /nologo /subsystem:console /entry:main %s", obj_filename);
    printf("%s\n", command);
    system(command);
    printf("Compiled successfully.");
    return 0;
}
