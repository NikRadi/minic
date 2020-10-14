#include "Codegenx86.h"


static void Codegenx86Block(FILE *file, Block *block);
static int Codegenx86Expr(FILE *file, Ast *expr, Bool is_jump, int label);


static Bool is_reg_free[4];
static char *regs[] = {"r8", "r9", "r10", "r11"};
static char *var_idents[32];
static char *compares[] = {"sete", "setne", "setl", "setle", "setg", "setge"};
static char *inverse_branch_compares[] = {"jne", "je", "jge", "jg", "gle", "ge"};
static int num_vars = 0;
static int num_labels = 0;


static void FreeRegs() {
    for (int i = 0; i < 4; ++i) {
        is_reg_free[i] = TRUE;
    }
}

static int AllocRegister() {
    for (int i = 0; i < 4; ++i) {
        if (is_reg_free[i]) {
            is_reg_free[i] = FALSE;
            return i;
        }
    }

    printf("internal error: out of registers\n");
    exit(1);
}

static int FindMemLocation(char *ident) {
    int regid = -1;
    for (int i = 0; i < num_vars; ++i) {
        if (strcmp(ident, var_idents[i]) == 0) {
            regid = i;
            break;
        }
    }

    if (regid == -1) {
        printf("internal error: could not find memory location of variable '%s'\n", ident);
        exit(1);
    }

    return regid;
}

static int NewLabel() {
    int id = num_labels;
    num_labels += 1;
    return id;
}

static int Codegenx86Literal(FILE *file, Literal *literal) {
    if (literal->info.type == AST_INT_LITERAL) {
        int regid = AllocRegister();
        fprintf(file,
            "\tmov\t\t%s, %d\n",
            regs[regid], literal->intvalue
        );

        return regid;
    }

    printf("internal error: invalid literal type '%d'\n", literal->info.type);
    exit(1);
}

static int Codegenx86BinaryOp(FILE *file, BinaryOp *binaryop, Bool is_jump, int label) {
    int regid_lhs = Codegenx86Expr(file, binaryop->lhs, FALSE, -1);
    int regid_rhs = Codegenx86Expr(file, binaryop->rhs, FALSE, -1);
    switch (binaryop->optype) {
        case OP_ISEQUAL:
        case OP_NOTEQUAL:
        case OP_ISLESS_THAN:
        case OP_ISLESS_THAN_EQUAL:
        case OP_ISGREATER_THAN:
        case OP_ISGREATER_THAN_EQUAL: {
            if (is_jump) {
                fprintf(file,
                    "\tcmp\t\t%s, %s\n"
                    "\t%s\t\t%d\n",
                    regs[regid_lhs], regs[regid_rhs],
                    inverse_branch_compares[binaryop->optype - OP_ISEQUAL], label
                );
            }
            else {
                fprintf(file,
                    "\tcmp\t\t%s, %s\n"
                    "\t%s\t%sb\n"
                    "\tand\t\t%s, 0xff\n",
                    regs[regid_lhs], regs[regid_rhs],
                    compares[binaryop->optype - OP_ISEQUAL], regs[regid_lhs],
                    regs[regid_lhs]
                );
            }
        } break;
        default: {
            printf("internal error: binary operator '%d' not implemented\n", binaryop->optype);
            exit(1);
        };
    }

    return 0;
}

static int Codegenx86Expr(FILE *file, Ast *expr, Bool is_jump, int label) {
    switch (expr->type) {
        case AST_INT_LITERAL: return Codegenx86Literal(file, (Literal *) expr);
        case AST_BINARYOP:    return Codegenx86BinaryOp(file, (BinaryOp *) expr, is_jump, label);
        default: {
            printf("internal error: invalid expression type '%d'\n", expr->type);
            exit(1);
        };
    }
}

static void Codegenx86VarDecl(FILE *file, VarDecl *vardecl) {
    var_idents[num_vars] = strdup(vardecl->ident);
    if (vardecl->expr != 0) {
        int regid = Codegenx86Expr(file, vardecl->expr, FALSE, -1);
        FreeRegs();
        fprintf(file,
            "\tmov\t\t[rbp-%d], %s\n",
            num_vars, regs[regid]
        );
    }

    num_vars += 1;
}

static void Codegenx86VarAssign(FILE *file, VarAssign *varassign) {
    int var_location = FindMemLocation(varassign->ident);
    int regid = Codegenx86Expr(file, varassign->expr, FALSE, -1);
    FreeRegs();
    fprintf(file,
        "\tmov\t\t[rbp-%d], %s\n",
        var_location, regs[regid]
    );
}

static void Codegenx86IfStmtElse(FILE *file, IfStmt *elsestmt, int end_label) {
    int false_label = NewLabel();
    if (elsestmt->condition != 0) {
        Codegenx86Expr(file, elsestmt->condition, TRUE, false_label);
    }

    Codegenx86Block(file, elsestmt->block);
    if (elsestmt->elsestmt != 0) {
        fprintf(file,
            "\tjmp\t\tL%d\n"
            "L%d:\n",
            end_label, false_label
        );

        Codegenx86IfStmtElse(file, elsestmt->elsestmt, end_label);
    }
}

static void Codegenx86IfStmt(FILE *file, IfStmt *ifstmt) {
    int false_label = NewLabel();
    int end_label = -1;
    Codegenx86Expr(file, ifstmt->condition, TRUE, false_label);
    Codegenx86Block(file, ifstmt->block);
    if (ifstmt->elsestmt != 0) {
        end_label = NewLabel();
        fprintf(file, "\tjmp\t\tL%d\n", end_label);
    }

    fprintf(file, "L%d:\n", false_label);
    if (ifstmt->elsestmt != 0) {
        Codegenx86IfStmtElse(file, ifstmt->elsestmt, end_label);
        fprintf(file, "L%d:\n", end_label);
    }
}

static void Codegenx86WhileLoop(FILE *file, WhileLoop *whileloop) {
    int start_label = NewLabel();
    int end_label = NewLabel();
    fprintf(file, "L%d\n", start_label);
    Codegenx86Expr(file, whileloop->condition, TRUE, end_label);
    Codegenx86Block(file, whileloop->block);
    fprintf(file,
        "\tjmp\t\tL%d\n"
        "L%d\n",
        start_label, end_label
    );
}

static void Codegenx86Block(FILE *file, Block *block) {
    Block *child_block = block;
    while (child_block != 0 && child_block->stmt != 0) {
        switch (child_block->stmt->type) {
            case AST_VARDECL:   {Codegenx86VarDecl(file, (VarDecl *) child_block->stmt);} break;
            case AST_VARASSIGN: {Codegenx86VarAssign(file, (VarAssign *) child_block->stmt);} break;
            case AST_IFSTMT:    {Codegenx86IfStmt(file, (IfStmt *) child_block->stmt);} break;
            case AST_WHILELOOP: {Codegenx86WhileLoop(file, (WhileLoop *) child_block->stmt);} break;
            default: {
                printf("internal error: invalid statement type '%d'\n", child_block->stmt->type);
                exit(1);
            };
        }

        child_block = child_block->glue;
    }
}

static void Codegenx86FuncDecl(FILE *file, FuncDecl *funcdecl) {
    int num_vars_in_func = 10;
    int bytes = 32 + (num_vars_in_func * 8);
    fprintf(file,
        "%s:\n"
        "\tpush\trbp\n"
        "\tmov\t\trbp, rsp\n"
        "\tsub\t\trsp, %d\n",
        funcdecl->ident,
        bytes
    );

    Codegenx86Block(file, funcdecl->block);
    fprintf(file,
        "\tadd\t\trsp, %d\n"
        "\txor\t\trax, rax\n",
        bytes
    );

    if (strcmp(funcdecl->ident, "main") == 0) {
        fputs("\tcall\tExitProcess", file);
    }
    else {
        fputs("\tret", file);
    }
}

void Codegenx86File(FILE *file, File *cfile) {
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
        "\tret",
        file
    );

    FreeRegs();
    File *child_file = cfile;
    while (child_file != 0 && child_file->funcdecl != 0) {
        fputs("\n\n", file);
        Codegenx86FuncDecl(file, child_file->funcdecl);
        child_file = child_file->glue;
    }
}
