#include "Codegenx86.h"


static void Codegenx86Block(FileInfo *info, Block *block);
static int Codegenx86Expr(FileInfo *info, Ast *expr, Bool is_jump, int label);


static Bool is_reg_free[4];
static char *regs[] = {"r8", "r9", "r10", "r11"};
static char *compares[] = {"sete", "setne", "setl", "setle", "setg", "setge"};
static char *inverse_branch_compares[] = {"jne", "je", "jge", "jg", "jle", "je"};
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

static int FindMemLocation(FileInfo *info, char *ident) {
    int regid = -1;
    for (int i = 0; i < info->num_vars; ++i) {
        if (strcmp(ident, info->var_idents[i]) == 0) {
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

static int Codegenx86Literal(FileInfo *info, Literal *literal) {
    if (literal->info.type == AST_LITERAL_INT) {
        int regid = AllocRegister();
        fprintf(info->asmfile,
            "\tmov\t\t%s, %d\n",
            regs[regid], literal->intvalue
        );

        return regid;
    }

    if (literal->info.type == AST_LITERAL_IDENT) {
        int regid = AllocRegister();
        int mem_location = FindMemLocation(info, literal->strvalue);
        fprintf(info->asmfile,
            "\tmov\t\t%s, [rbp-%d]\n",
            regs[regid], mem_location
        );

        return regid;
    }

    printf("internal error: invalid literal type '%d'\n", literal->info.type);
    exit(1);
}

static int Codegenx86BinaryOp(FileInfo *info, BinaryOp *binaryop, Bool is_jump, int label) {
    int regid_lhs = Codegenx86Expr(info, binaryop->lhs, FALSE, -1);
    int regid_rhs = Codegenx86Expr(info, binaryop->rhs, FALSE, -1);
    switch (binaryop->optype) {
        case OP_ADD: {fprintf(info->asmfile, "\tadd\t\t%s, %s\n", regs[regid_lhs], regs[regid_rhs]);} break;
        case OP_ISEQUAL:
        case OP_NOTEQUAL:
        case OP_ISLESS_THAN:
        case OP_ISLESS_THAN_EQUAL:
        case OP_ISGREATER_THAN:
        case OP_ISGREATER_THAN_EQUAL: {
            if (is_jump) {
                fprintf(info->asmfile,
                    "\tcmp\t\t%s, %s\n"
                    "\t%s\t\tL%d\n",
                    regs[regid_lhs], regs[regid_rhs],
                    inverse_branch_compares[binaryop->optype - OP_ISEQUAL], label
                );
            }
            else {
                fprintf(info->asmfile,
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

static int Codegenx86Expr(FileInfo *info, Ast *expr, Bool is_jump, int label) {
    switch (expr->type) {
        case AST_LITERAL_INT:
        case AST_LITERAL_IDENT: return Codegenx86Literal(info, (Literal *) expr);
        case AST_BINARYOP:      return Codegenx86BinaryOp(info, (BinaryOp *) expr, is_jump, label);
        default: {
            printf("internal error: invalid expression type '%d'\n", expr->type);
            exit(1);
        };
    }
}

static void Codegenx86VarDecl(FileInfo *info, VarDecl *vardecl) {
    info->var_idents[info->num_vars] = vardecl->ident;
    if (vardecl->expr != 0) {
        int regid = Codegenx86Expr(info, vardecl->expr, FALSE, -1);
        FreeRegs();
        fprintf(info->asmfile,
            "\tmov\t\t[rbp-%d], %s\n",
            info->num_vars, regs[regid]
        );
    }

    info->num_vars += 1;
}

static void Codegenx86VarAssign(FileInfo *info, VarAssign *varassign) {
    int var_location = FindMemLocation(info, varassign->ident);
    int regid = Codegenx86Expr(info, varassign->expr, FALSE, -1);
    FreeRegs();
    fprintf(info->asmfile,
        "\tmov\t\t[rbp-%d], %s\n",
        var_location, regs[regid]
    );
}

static void Codegenx86IfStmtElse(FileInfo *info, IfStmt *elsestmt, int end_label) {
    Bool has_elsestmt = elsestmt->elsestmt != 0;
    int false_label = (has_elsestmt) ? NewLabel() : end_label;
    if (elsestmt->condition != 0) {
        Codegenx86Expr(info, elsestmt->condition, TRUE, false_label);
        FreeRegs();
    }

    Codegenx86Block(info, elsestmt->block);
    if (has_elsestmt) {
        fprintf(info->asmfile,
            "\tjmp\t\tL%d\n"
            "L%d:\n",
            end_label, false_label
        );

        Codegenx86IfStmtElse(info, elsestmt->elsestmt, end_label);
    }
}

static void Codegenx86IfStmt(FileInfo *info, IfStmt *ifstmt) {
    int false_label = NewLabel();
    int end_label = -1;
    Codegenx86Expr(info, ifstmt->condition, TRUE, false_label);
    FreeRegs();
    Codegenx86Block(info, ifstmt->block);
    if (ifstmt->elsestmt != 0) {
        end_label = NewLabel();
        fprintf(info->asmfile, "\tjmp\t\tL%d\n", end_label);
    }

    fprintf(info->asmfile, "L%d:\n", false_label);
    if (ifstmt->elsestmt != 0) {
        Codegenx86IfStmtElse(info, ifstmt->elsestmt, end_label);
        fprintf(info->asmfile, "L%d:\n", end_label);
    }
}

static void Codegenx86WhileLoop(FileInfo *info, WhileLoop *whileloop) {
    int start_label = NewLabel();
    int end_label = NewLabel();
    fprintf(info->asmfile, "L%d:\n", start_label);
    Codegenx86Expr(info, whileloop->condition, TRUE, end_label);
    FreeRegs();
    Codegenx86Block(info, whileloop->block);
    fprintf(info->asmfile,
        "\tjmp\t\tL%d\n"
        "L%d:\n",
        start_label, end_label
    );
}

static void Codegenx86FuncCall(FileInfo *info, FuncCall *funccall) {
    if (funccall->arg != 0) {
        int regid = Codegenx86Expr(info, funccall->arg, FALSE, -1);
        FreeRegs();
        fprintf(info->asmfile, "\tmov\t\trcx, %s\n", regs[regid]);
    }

    fprintf(info->asmfile, "\tcall\t%s\n", funccall->ident);
}

static void Codegenx86Block(FileInfo *info, Block *block) {
    Block *child_block = block;
    while (child_block != 0 && child_block->stmt != 0) {
        switch (child_block->stmt->type) {
            case AST_VARDECL:   {Codegenx86VarDecl(info, (VarDecl *) child_block->stmt);} break;
            case AST_VARASSIGN: {Codegenx86VarAssign(info, (VarAssign *) child_block->stmt);} break;
            case AST_IFSTMT:    {Codegenx86IfStmt(info, (IfStmt *) child_block->stmt);} break;
            case AST_WHILELOOP: {Codegenx86WhileLoop(info, (WhileLoop *) child_block->stmt);} break;
            case AST_FUNCCALL:  {Codegenx86FuncCall(info, (FuncCall *) child_block->stmt);} break;
            default: {
                printf("internal error: invalid statement type '%d'\n", child_block->stmt->type);
                exit(1);
            };
        }

        child_block = child_block->glue;
    }
}

static void Codegenx86FuncDecl(FileInfo *info, FuncDecl *funcdecl) {
    int bytes = 32 + funcdecl->stack_depth_bytes;
    fprintf(info->asmfile,
        "%s:\n"
        "\tpush\trbp\n"
        "\tmov\t\trbp, rsp\n"
        "\tsub\t\trsp, %d\n",
        funcdecl->ident,
        bytes
    );

    Codegenx86Block(info, funcdecl->block);
    fprintf(info->asmfile,
        "\tadd\t\trsp, %d\n"
        "\txor\t\trax, rax\n",
        bytes
    );

    if (strcmp(funcdecl->ident, "main") == 0) {
        fputs("\tcall\tExitProcess", info->asmfile);
    }
    else {
        fputs("\tret", info->asmfile);
    }
}

void Codegenx86File(FileInfo *info, File *cfile) {
    fputs(
        "bits 64\n"
        "default rel\n"
        "\n"
        "segment .data\n"
        "\tfmt:\tdb \"%d\", 0xd, 0xa, 0\n"
        "\n"
        "segment .text\n"
        "\tglobal main\n"
        "\textern ExitProcess\n"
        "\textern printf\n"
        "\n"
        "PrintInt:\n"
        "\tsub\t\trsp, 32\n"
        "\tmov\t\trdx, rcx\n"
        "\tlea\t\trcx, [fmt]\n"
        "\tcall\tprintf\n"
        "\tadd\t\trsp, 32\n"
        "\tret",
        info->asmfile
    );

    FreeRegs();
    File *child_file = cfile;
    while (child_file != 0 && child_file->funcdecl != 0) {
        fputs("\n\n", info->asmfile);
        Codegenx86FuncDecl(info, child_file->funcdecl);
        child_file = child_file->glue;
    }
}
