#ifndef MINIC_TYPECHECKER_H
#define MINIC_TYPECHECKER_H
#include "Parser.h"


struct VarInfo {
    DataType datatype;
    char *ident;
} typedef VarInfo;

struct FileInfo {
    int num_funcs;
    int num_vars;
    FuncDecl *funcs[16];
    VarInfo var_infos[16];
    char *filename;
    FuncDecl *current_func;
    FILE *asmfile;
} typedef FileInfo;


void TypecheckFile(FileInfo *info, File *file);

#endif // MINIC_TYPECHECKER_H