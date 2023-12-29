#ifndef MINIC_ASSEMBLY_H
#define MINIC_ASSEMBLY_H
#include <stdio.h>

void Add(char *destination, char *source);

void Compare(char *a, char *b, char *comparison);

void Div(char *operand);

void Jmp(char *label);

void Label(char *name);

void Lea(char *destination, int rbp_offset);

void Mov(char *destination, char *source);

void MovImmediate(char *destination, int value);

void Mul(char *destination, char *source);

void Neg(char *destination);

void Pop(char *destination);

void Push(char *source);

void RestoreStackFrame();

void SetOutput(FILE *file);

void SetupAssemblyFile(char *entry);

void SetupStackFrame(int size);

void Sub(char *destination, char *source);

#endif // MINIC_ASSEMBLY_H
