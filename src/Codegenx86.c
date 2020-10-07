#include "Codegenx86.h"

static enum Bool is_reg_free[4];
static char *regs[4] = {"r8", "r9", "r10", "r11"};

static void FreeRegisters() {
    for (int i = 0; i < 4; ++i) {
        is_reg_free[i] = TRUE;
    }
}

static void FreeRegister(int regid) {
    is_reg_free[regid] = TRUE;
}

static int AllocRegister() {
    for (int i = 0; i < 4; ++i) {
        if (is_reg_free[i]) {
            is_reg_free[i] = FALSE;
            return i;
        }
    }

    printf("out of registers\n");
    exit(1);
}

static int Codegenx86Expr(FILE *file, struct AstNode *expr) {
    if (expr->type == AST_INT_LITERAL) {
        int regid = AllocRegister();
        fprintf(file,
            "\tmov\t\t%s, %d\n",
            regs[regid],
            expr->intvalue
        );

        return regid;
    }
    else {
        int regid1 = Codegenx86Expr(file, expr->lhs);
        int regid2 = Codegenx86Expr(file, expr->rhs);
        switch (expr->type) {
            case AST_ADD: {
                fprintf(file, "\tadd\t\t%s, %s\n", regs[regid1], regs[regid2]);
            } break;
            case AST_SUB: {
                fprintf(file, "\tsub\t\t%s, %s\n", regs[regid1], regs[regid2]);
            } break;
            case AST_MUL: {
                fprintf(file, "\timul\t%s, %s\n", regs[regid1], regs[regid2]);
            } break;
            default: {
                printf("unknown operator type\n");
                exit(1);
            } break;
        }

        FreeRegister(regid2);
        return regid1;
    }
}

void Codegenx86(FILE *file, struct AstNode *ast) {
    fputs(
        "bits 64\n"
        "default rel\n"
        "\n"
        "segment .data\n"
        "\tstr: db \"%d\", 0xd, 0xa, 0\n"
        "\n"
        "segment .text\n"
        "global main\n"
        "extern ExitProcess\n"
        "extern printf\n"
        "\n"
        "main:\n",
        file
    );

    FreeRegisters();
    int regid = Codegenx86Expr(file, ast);
    fprintf(file, "\tmov\t\trdx, %s\n", regs[regid]);
    fputs(
        "\tlea\t\trcx, [str]\n"
        "\tcall\tprintf\n"
        "\txor\t\trax, rax\n"
        "\tcall\tExitProcess",
        file
    );
}