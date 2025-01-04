#include "Assembly.h"

static FILE *f;

void Add(char *destination, char *source) {
    fprintf(f, "  add %s, %s\n", destination, source);
}

void Call(char *label) {
    fprintf(f, "  call %s\n", label);
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

void Lea(char *destination, int rbp_offset) {
    fprintf(f, "  lea %s, [rbp - %d]\n", destination, rbp_offset);
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

void SetupAssemblyFile(char *entry) {
    fprintf(f,
        // Set the assembly to use 64-bit mode.
        "bits 64\n"
        // Set the ddefault operand size to be relative.
        "default rel\n"
        "\n"
        // Define the .text segment, which stores the instruction code.
        "segment .text\n"
        // Declare the main function as the entry point of the program.
        "  global %s\n"
        "\n"
        "ret7:\n"
        "  mov rax, 7\n"
        "  ret\n"
        "ret14:\n"
        "  mov rax, 14\n"
        "  ret\n"
        "sub:\n"
        "  mov rax, 0\n"
        "  mov rax, rdi\n"
        "  sub rax, rsi\n"
        "  ret\n"
        "add:\n"
        "  mov rax, 0\n"
        "  add rax, rdi\n"
        "  add rax, rsi\n"
        "  ret\n"
        "add6:\n"
        "  mov rax, 0\n"
        "  add rax, rdi\n"
        "  add rax, rsi\n"
        "  add rax, rdx\n"
        "  add rax, rcx\n"
        "  add rax, r8\n"
        "  add rax, r9\n"
        "  ret\n"
        "\n",
        entry
    );
}

void SetupStackFrame(int size) {
    fprintf(f,
        "  push rbp\n"
        "  mov rbp, rsp\n"
    );

    if (size > 0) {
        fprintf(f, "  sub rsp, %d\n", size);
    }
}

void Sub(char *destination, char *source) {
    fprintf(f, "  sub %s, %s\n", destination, source);
}
