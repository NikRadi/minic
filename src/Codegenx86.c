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

static void Codegenx86PrintStmt(FILE *file, struct AstNode *print_stmt) {
    int regid = Codegenx86Expr(file, print_stmt->lhs);
    FreeRegisters();
    fprintf(file,
        "\tmov\t\trcx, %s\n"
        "\tcall\tprintint\n",
        regs[regid]
    );
}

static int num_vars = 0;
static char *vars[8];
static void Codegenx86CompoundStmt(FILE *file, struct AstNode *compund_stmt) {
    struct AstNode *current_compound_stmt = compund_stmt;
    while (TRUE) {
        if (current_compound_stmt->lhs == 0) {
            break;
        }

        switch (current_compound_stmt->lhs->type) {
            case AST_PRINT: {
                Codegenx86PrintStmt(file, current_compound_stmt->lhs);
            } break;
            case AST_DECL: {
                vars[num_vars] = strdup(current_compound_stmt->strvalue);
                num_vars += 1;
            } break;
            case AST_ASSIGN: {

            } break;
            default: {
                printf("internal error, invalid statement in compound\n");
                exit(1);
            } break;
        }

        if (current_compound_stmt->rhs == 0) {
            break;
        }

        current_compound_stmt = current_compound_stmt->rhs;
    }
}

void Codegenx86(FILE *file, struct AstNode *ast) {
    fputs(
        "bits 64\n"
        "default rel\n"
        "\n"
        "segment .data\n"
        "\tfmt: db \"%d\", 0xd, 0xa, 0\n"
        "\n"
        "segment .text\n"
        "global main\n"
        "extern ExitProcess\n"
        "extern printf\n"
        "\n"
        "printint:\n"
        "\tsub\t\trsp, 28h\n"
        "\tmov\t\trdx, rcx\n"
        "\tlea\t\trcx, [fmt]\n"
        "\tcall\tprintf\n"
        "\tadd\t\trsp, 28h\n"
        "\tret\n"
        "\n"
        "main:\n",
        file
    );

    FreeRegisters();
    Codegenx86CompoundStmt(file, ast);

    fputs(
        "\txor\t\trax, rax\n"
        "\tcall\tExitProcess",
        file
    );
}