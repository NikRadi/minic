#include "Codegenx86.h"


static void Codegenx86CompoundStmt(FILE *file, struct AstNode *compound_stmt);


static enum Bool is_reg_free[4];
static char *regs[] = {"r8", "r9", "r10", "r11"};
static char *inverse_branch_compares[] = {"jne", "je", "jge", "jg", "gle", "ge"};
static char *compares[] = {"sete", "setne", "setl", "setle", "setg", "setge"};
static int num_vars = 0;
static char *vars[8];
static int labelid = 0;


static int NewLabel() {
    int id = labelid;
    labelid += 1;
    return id;
}

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

static int Codegenx86Expr(FILE *file, struct AstNode *expr, enum AstNodeType parent_ast, int label) {
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
        // If we recurse into rhs first we don't run out of registers
        int regid1 = Codegenx86Expr(file, expr->lhs, expr->type, -1);
        int regid2 = Codegenx86Expr(file, expr->rhs, expr->type, -1);
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
            case AST_ISEQUAL:
            case AST_NOTEQUAL:
            case AST_ISLESS_THAN:
            case AST_ISLESS_THAN_EQUAL:
            case AST_ISGREATER_THAN:
            case AST_ISGREATER_THAN_EQUAL: {
                if (parent_ast == AST_IF || parent_ast == AST_WHILE) {
                    fprintf(file,
                        "\tcmp\t\t%s, %s\n"
                        "\t%s\t\tL%d\n",
                        regs[regid1], regs[regid2],
                        inverse_branch_compares[expr->type - AST_ISEQUAL], label
                    );
                }
                else {
                    fprintf(file,
                        "\tcmp\t\t%s, %s\n"
                        "\t%s\t%sb\n"
                        "\tand\t\t%s, 0xff\n",
                        regs[regid1], regs[regid2],
                        compares[expr->type - AST_ISEQUAL], regs[regid1],
                        regs[regid1]
                    );
                }
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
    int regid = Codegenx86Expr(file, print_stmt->lhs, AST_PRINT, -1);
    FreeRegisters();
    fprintf(file,
        "\tmov\t\trcx, %s\n"
        "\tcall\tprintint\n",
        regs[regid]
    );
}

static void Codegenx86VarAssign(FILE *file, struct AstNode *varassign) {
    char *varident = varassign->lhs->strvalue;
    int identid = FindMemLocation(varident);
    int regid = Codegenx86Expr(file, varassign->rhs, AST_ASSIGN, -1);
    FreeRegisters();
    fprintf(file,
        "\tmov\t\t[rbp-%d], %s\n",
        identid,
        regs[regid]
    );
}

static void Codegenx86IfStmt(FILE *file, struct AstNode *ifstmt) {
    enum Bool has_else = ifstmt->rhs->rhs != 0;
    int false_label = NewLabel();
    int end_label = -1;
    Codegenx86Expr(file, ifstmt->lhs, AST_IF, false_label);
    Codegenx86CompoundStmt(file, ifstmt->rhs->lhs);
    if (has_else) {
        end_label = NewLabel();
        fprintf(file, "\tjmp\t\tL%d\n", end_label);
    }

    fprintf(file, "L%d:\n", false_label);
    FreeRegisters();
    if (has_else) {
        Codegenx86CompoundStmt(file, ifstmt->rhs->rhs);
        fprintf(file, "L%d:\n", end_label);
    }
}

static void Codegenx86WhileLoop(FILE *file, struct AstNode *whileloop) {
    int start_label = NewLabel();
    int end_label = NewLabel();
    fprintf(file, "L%d:\n", start_label);
    Codegenx86Expr(file, whileloop->lhs, AST_WHILE, end_label);
    Codegenx86CompoundStmt(file, whileloop->rhs);
    fprintf(file,
        "\tjmp\t\tL%d\n"
        "L%d:\n",
        start_label,
        end_label
    );
}

static void Codegenx86CompoundStmt(FILE *file, struct AstNode *compund_stmt) {
    struct AstNode *current_compound_stmt = compund_stmt;
    while (TRUE) {
        if (current_compound_stmt->lhs == 0) {
            break;
        }

        switch (current_compound_stmt->lhs->type) {
            case AST_PRINT:  {Codegenx86PrintStmt(file, current_compound_stmt->lhs);} break;
            case AST_ASSIGN: {Codegenx86VarAssign(file, current_compound_stmt->lhs);} break;
            case AST_IF:     {Codegenx86IfStmt(file, current_compound_stmt->lhs);} break;
            case AST_WHILE:  {Codegenx86WhileLoop(file, current_compound_stmt->lhs);} break;
            case AST_DECL: {
                vars[num_vars] = strdup(current_compound_stmt->lhs->strvalue);
                num_vars += 1;
            } break;
            default: {
                printf("internal error, invalid statement in compound '%d'\n", current_compound_stmt->lhs->type);
                exit(1);
            } break;
        }

        if (current_compound_stmt->rhs == 0) {
            break;
        }

        current_compound_stmt = current_compound_stmt->rhs;
    }
}

static void Codegenx86FuncDecl(FILE *file, struct AstNode *funcdecl) {
    fprintf(file,
        "%s:\n"
        "\tpush\trbp\n"
        "\tmov\t\trbp, rsp\n"
        "\tsub\t\trsp, 48\n",
        funcdecl->lhs->strvalue
    );

    Codegenx86CompoundStmt(file, funcdecl->rhs);
    fputs(
        "\tadd\t\trsp, 48\n"
        "\txor\t\trax, rax\n",
        file
    );

    if (strcmp(funcdecl->lhs->strvalue, "main") == 0) {
        fputs("\tcall\tExitProcess\n\n", file);
    }
}

void Codegenx86File(FILE *file, struct AstNode *ast) {
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
        "\n",
        file
    );

    FreeRegisters();
    struct AstNode *stmt = ast;
    while (stmt != 0 && stmt->lhs != 0) {
        switch (stmt->lhs->type) {
            case AST_FUNCTION: {Codegenx86FuncDecl(file, stmt->lhs);} break;
        }

        stmt = stmt->rhs;
    }
}