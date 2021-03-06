#include <stdio.h>
#include <stdlib.h>
#include "CodegenX86.h"
#include "Common.h"
#include "DebugPrint.h"
#include "ErrorPrint.h"
#include "Lexer.h"
#include "Parser.h"
#include "Typechecker.h"

struct Args {
    char *filename;
    Bool print_tokens;
    Bool print_ast;
} typedef Args;


static Args ParseArgs(int argc, char **argv) {
    Args args;
    args.filename = 0;
    args.print_tokens = FALSE;
    args.print_ast = FALSE;
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-tok") == 0) {
            args.print_tokens = TRUE;
        }
        else if (strcmp(argv[i], "-ast") == 0) {
            args.print_ast = TRUE;
        }
        else {
            args.filename = argv[i];
        }
    }

    return args;
}

int main(int argc, char **argv) {
    Args args = ParseArgs(argc, argv);
    if (args.filename == 0) {
        ThrowFatalError("no file name given");
    }

    FILE *sourcecode = fopen(args.filename, "rb");
    if (sourcecode == NULL) {
        ThrowFatalError("could not read from file '%s'", args.filename);
    }

    fseek(sourcecode, 0, SEEK_END);
    Lexer lexer;
    lexer.line = 1;
    lexer.char_idx = 0;
    lexer.text_size = ftell(sourcecode);
    lexer.filename = args.filename;
    rewind(sourcecode);
    lexer.text = malloc(lexer.text_size);
    fread(lexer.text, 1, lexer.text_size, sourcecode);
    ReadToken(&lexer);
    ReadToken(&lexer);
    if (args.print_tokens) {
        printf("Printing tokens...\n");
        while (TRUE) {
            ReadToken(&lexer);
            PrintToken(lexer.token);
            printf("\n");
            if (lexer.token.type == TOKEN_EOF) {
                return 0;
            }
        }
    }

    File *file = ParseFile(&lexer);
    if (args.print_ast) {
        printf("Printing ast...\n");
        PrintFile(file);
    }

    FileInfo info;
    info.num_funcs = 0;
    info.filename = lexer.filename;
    info.current_func = NULL;
    TypecheckFile(&info, file);

    size_t filename_len = strlen(args.filename);
    char *asmfilename = (char *) malloc(filename_len + 3); // len - c + asm + \0
    strncpy(asmfilename, args.filename, filename_len - 1);
    asmfilename[filename_len - 1] = 'a';
    asmfilename[filename_len - 0] = 's';
    asmfilename[filename_len + 1] = 'm';
    asmfilename[filename_len + 2] = '\0';
    info.asmfile = fopen(asmfilename, "w");
    if (info.asmfile == NULL) {
        ThrowFatalError("could not write to file '%s'", asmfilename);
    }

    CgX86File(&info, file);
    fclose(info.asmfile);
    return 0;
}
