#ifndef MINIC_CODE_GENRATOR_X86_H
#define MINIC_CODE_GENRATOR_X86_H
#include "AstNode.h"
#include "List.h"
#include <stdbool.h>
#include <stdio.h>

void CodeGeneratorX86_GenerateCode(FILE *asm_file, struct FunctionDefinition *function);

#endif // MINIC_CODE_GENRATOR_X86_H
