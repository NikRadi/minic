#ifndef MINIC_SIZES_H
#define MINIC_SIZES_H
#include "AstNode.h"

#define AL "al"
#define EAX "eax"
#define RAX "rax"

#define DIL "dil"
#define EDI "edi"
#define RDI "rdi"

#define CL "cl"
#define ECX "ecx"
#define RCX "rcx"

#define DL "dl"
#define EDX "edx"
#define RDX "rdx"

#define R8B "r8b"
#define R8D "r8d"
#define R8 "r8"

#define R9B "r9b"
#define R9D "r9d"
#define R9 "r9"

#define BYTE "byte"
#define WORD "word"
#define DWORD "dword"
#define QWORD "qword"


static char bytes[PRIMTYPE_COUNT] = {
    [PRIMTYPE_INVALID]  = 0,
    [PRIMTYPE_CHAR]     = 1,
    [PRIMTYPE_INT]      = 4,
    [PRIMTYPE_PTR]      = 8,
};

static char *size[PRIMTYPE_COUNT] = {
    [PRIMTYPE_INVALID]  = "INVALID(size)",
    [PRIMTYPE_CHAR]     = BYTE,
    [PRIMTYPE_INT]      = DWORD,
    [PRIMTYPE_PTR]      = QWORD,
};

static char *rax[PRIMTYPE_COUNT] = {
    [PRIMTYPE_INVALID]  = "INVALID(rax)",
    [PRIMTYPE_CHAR]     = AL,
    [PRIMTYPE_INT]      = EAX,
    [PRIMTYPE_PTR]      = RAX,
};

static char *rdi[PRIMTYPE_COUNT] = {
    [PRIMTYPE_INVALID]  = "INVALID(rdi)",
    [PRIMTYPE_CHAR]     = DIL,
    [PRIMTYPE_INT]      = EDI,
    [PRIMTYPE_PTR]      = RDI,
};

static char *rcx[PRIMTYPE_COUNT] = {
    [PRIMTYPE_INVALID]  = "INVALID(rcx)",
    [PRIMTYPE_CHAR]     = CL,
    [PRIMTYPE_INT]      = ECX,
    [PRIMTYPE_PTR]      = RCX,
};

static char *rdx[PRIMTYPE_COUNT] = {
    [PRIMTYPE_INVALID]  = "INVALID(rdx)",
    [PRIMTYPE_CHAR]     = DL,
    [PRIMTYPE_INT]      = EDX,
    [PRIMTYPE_PTR]      = RDX,
};

static char *r8[PRIMTYPE_COUNT] = {
    [PRIMTYPE_INVALID]  = "INVALID(r8)",
    [PRIMTYPE_CHAR]     = R8B,
    [PRIMTYPE_INT]      = R8D,
    [PRIMTYPE_PTR]      = R8,
};

static char *r9[PRIMTYPE_COUNT] = {
    [PRIMTYPE_INVALID]  = "INVALID(r9)",
    [PRIMTYPE_CHAR]     = R9B,
    [PRIMTYPE_INT]      = R9D,
    [PRIMTYPE_PTR]      = R9,
};


// The first 4 Win64 function parameters go to these registers.
// Additional parameters must be pushed to the stack.
// https://www.cs.uaf.edu/2017/fall/cs301/reference/x86_64.html
static char **param_regs[4] = { rcx, rdx, r8, r9 };

#endif // MINIC_SIZES_H
