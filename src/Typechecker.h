#ifndef MINIC_TYPECHECKER_H
#define MINIC_TYPECHECKER_H
#include "Parser.h"


struct FileInfo {
    int num_funcs;
    int num_vars;
    char *func_idents[16];
    char *var_idents[16];
    char *filename;
    FuncDecl *current_func;
    FILE *asmfile;
} typedef FileInfo;


void TypecheckFile(FileInfo *info, File *file);

#endif // MINIC_TYPECHECKER_H