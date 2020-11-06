#ifndef MINIC_TYPECHECKER_H
#define MINIC_TYPECHECKER_H
#include "Parser.h"

struct FileInfo {
    int num_funcs;
    FuncDecl *funcs[16];
    char *filename;
    FuncDecl *current_func;
    Scope *current_scope;
    FILE *asmfile;
} typedef FileInfo;


void TypecheckFile(FileInfo *info, File *file);

#endif // MINIC_TYPECHECKER_H