#include "Codegenx86.h"

static enum Bool is_reg_free[4];
static char *regs[4] = {"r8", "r9", "r10", "r11"};
static char *bregs[4] = {"r8b", "r9b", "r10b", "r11b"};
static int num_vars = 0;
static char *vars[8];

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

    printf("internal error, out of registers\n");
    exit(1);
}

static int FindMemLocation(char *ident) {
        int identid = -1;
        for (int i = 0; i < num_vars; ++i) {
            if (strcmp(ident, vars[i]) == 0) {
                identid = i;
                break;
            }
        }

        if (identid == -1) {
            printf("internal error, could not find identifier '%s'\n", ident);
            exit(1);
        }

        return identid * 8;
}

static int Compare(FILE *file, int regid1, int regid2, char *asm) {
    fprintf(file,
        "\tcmp\t\t%s, %s\n"
        "\t%s\t%s\n"
        "\tand\t\t%s, 255\n",
        regs[regid1], regs[regid2],
        asm, bregs[regid1],
        regs[regid1]
    );

    FreeRegister(regid2);
    return regid1;
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
    else if (expr->type == AST_IDENT) {
        int identid = FindMemLocation(expr->strvalue);
        int regid = AllocRegister();
        fprintf(file,
            "\tmov\t\t%s, [rbp-%d]\n",
            regs[regid],
            identid
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
            case AST_ISEQUAL:               return Compare(file, regid1, regid2, "sete");
            case AST_NOTEQUAL:              return Compare(file, regid1, regid2, "setne");
            case AST_ISLESS_THAN:           return Compare(file, regid1, regid2, "setl");
            case AST_ISLESS_THAN_EQUAL:     return Compare(file, regid1, regid2, "setle");
            case AST_ISGREATER_THAN:        return Compare(file, regid1, regid2, "setg");
            case AST_ISGREATER_THAN_EQUAL:  return Compare(file, regid1, regid2, "setge");
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
                vars[num_vars] = strdup(current_compound_stmt->lhs->strvalue);
                num_vars += 1;
            } break;
            case AST_ASSIGN: {
                struct AstNode *varassign = current_compound_stmt->lhs;
                char *varident = varassign->lhs->strvalue;
                int identid = FindMemLocation(varident);
                int regid = Codegenx86Expr(file, varassign->rhs);
                FreeRegisters();
                fprintf(file,
                    "\tmov\t\t[rbp-%d], %s\n",
                    identid,
                    regs[regid]
                );
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
        "\tfmt:\tdb \"%d\", 0xd, 0xa, 0\n"
        "\n"
        "segment .text\n"
        "global main\n"
        "extern ExitProcess\n"
        "extern printf\n"
        "\n"
        "printint:\n"
        "\tsub\t\trsp, 32\n"
        "\tmov\t\trdx, rcx\n"
        "\tlea\t\trcx, [fmt]\n"
        "\tcall\tprintf\n"
        "\tadd\t\trsp, 32\n"
        "\tret\n"
        "\n"
        "main:\n"
        "\tpush\trbp\n"
        "\tmov\t\trbp, rsp\n"
        "\tsub\t\trsp, 48\n",
        file
    );

    FreeRegisters();
    Codegenx86CompoundStmt(file, ast);

    fputs(
        "\tadd\t\trsp, 48\n"
        "\txor\t\trax, rax\n"
        "\tcall\tExitProcess",
        file
    );
}