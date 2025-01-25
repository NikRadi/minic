#include "Assembly.h"
#include <assert.h>

static FILE *f;

void Add(char *destination, char *source) {
    fprintf(f, "  add %s, %s\n", destination, source);
}

void Call(char *label) {
    fprintf(f, "  call %s\n", label);
}

void Comment(char *comment) {
    fprintf(f, "  ; %s\n", comment);
}

void Compare(char *a, char *b, char *comparison) {
    fprintf(f,
        "  cmp %s, %s\n"
        // Store comparison instruction (e.g. sete, setne, etc.) result in 'al' (Lower 8 bits of rax).
        "  %s al\n",
        a, b,
        comparison
    );
}

void Div(char *operand) {
    fprintf(f,
        // Prepares for a signed division (convert quadword to octaword).
        "  cqo\n"
        // Divide rdx:rax by operand. Quotient goes to rax, remainder goes to rdx.
        // rdx:rax means rdx for the most significant bits and rax for the least significant bits.
        // Together, they form a single 64-bit value.
        "  idiv %s\n",
        operand
    );
}

void Jmp(char *label) {
    fprintf(f, "  jmp %s\n", label);
}

void Label(char *name) {
    fprintf(f, "%s:\n", name);
}

void Lea(char *dest, int rbp_offset) {
    fprintf(f, "  lea %s, [rbp - %d]\n", dest, rbp_offset);
}

void LoadMem(enum PrimitiveType primtype) {
    if (primtype == PRIMTYPE_CHAR) {
        fprintf(f, "  movzx rax, %s [rax]\n", size[primtype]);
    }
    else {
        fprintf(f, "  mov %s, %s [rax]\n", rax[primtype], size[primtype]);
    }
}

void Mov(char *destination, char *source) {
    fprintf(f, "  mov %s, %s\n", destination, source);
}

void MovImm(char *destination, int value) {
    fprintf(f, "  mov %s, %d\n", destination, value);
}

void Mul(char *destination, char *source) {
    fprintf(f, "  imul %s, %s\n", destination, source);
}

void Neg(char *destination) {
    fprintf(f, "  neg %s\n", destination);
}

void Pop(char *destination) {
    fprintf(f, "  pop %s\n", destination);
}

void Push(char *source) {
    fprintf(f, "  push %s\n", source);
}

void RestoreStackFrame() {
    fprintf(f,
        "  mov rsp, rbp\n"
        "  pop rbp\n"
        "  ret\n"
    );
}

void SetOutput(FILE *file) {
    f = file;
}

void SetupAssemblyFile() {
    fprintf(f,
        // Set the assembly to use 64-bit mode.
        "bits 64\n"
        // Set the ddefault operand size to be relative.
        "default rel\n"
        "\n"
    );
}

void SetupStackFrame(int stack_size) {
    fprintf(f,
        "  push rbp\n"
        "  mov rbp, rsp\n"
    );

    if (stack_size > 0) {
        fprintf(f, "  sub rsp, %d\n", stack_size);
    }
}

void Sub(char *destination, char *source) {
    fprintf(f, "  sub %s, %s\n", destination, source);
}

void WriteMemOffset(int rbp_offset, int reg_idx, enum PrimitiveType primtype) {
    assert(0 <= reg_idx && reg_idx < 4);
    char **param_reg = param_regs[reg_idx];
    char *reg = param_reg[primtype];
    fprintf(f, "  mov [rbp - %d], %s\n", rbp_offset, reg);
}

void WriteMemToReg(char *dest, char *src) {
    fprintf(f, "  mov [%s], %s\n", dest, src);
}
