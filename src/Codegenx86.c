#include "Codegenx86.h"


void Codegenx86(FILE *file, struct AstNode *ast) {
    fputs(
        "bits 64\n"
        "default rel\n"
        "\n"
        "segment .data\n"
        "\tt db \"%d\", 0xd, 0xa, 0\n"
        "\n"
        "segment .text\n"
        "global main\n"
        "extern ExitProcess\n"
        "extern printf\n"
        "\n"
        "main:\n"
        "\tmov\t\tedx, 123\n"
        "\tlea\t\trcx, [t]\n"
        "\tcall\tprintf\n"
        "\txor\t\trax, rax\n"
        "\tcall\tExitProcess",
        file
    );
}