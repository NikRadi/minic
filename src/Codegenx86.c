#include "Codegenx86.h"


static void CgX86Block(FileInfo *info, Block *block);
static int CgX86Expr(FileInfo *info, Ast *expr, Bool is_jump, int label);
static void CgX86FuncCall(FileInfo *info, FuncCall *funccall);


static Bool is_reg_free[4];
static char *regs64[] = {"r8", "r9", "r10", "r11"};
static char *regs32[] = {"r8d", "r9d", "r10d", "r11d"};
static char *regs8[] = {"r8b", "r9b", "r10b", "r11b"};
static char *compares[] = {"sete", "setne", "setl", "setle", "setg", "setge"};
static char *inverse_branch_compares[] = {"jne", "je", "jge", "jg", "jle", "je"};
static int num_labels = 0;


static void FreeRegs() {
    for (int i = 0; i < 4; ++i) {
        is_reg_free[i] = TRUE;
    }
}

static void FreeReg(int regid) {
    is_reg_free[regid] = TRUE;
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

static VarInfo GetVarInfo(FileInfo *info, char *ident) {
    for (int i = 0; i < info->num_vars; ++i) {
        if (strcmp(ident, info->var_infos[i].ident) == 0) {
            return info->var_infos[i];
        }
    }

    printf("internal error: could not find variable '%s'\n", ident);
    exit(1);
}

static int FindMemLocation(FileInfo *info, char *ident) {
    int regid = -1;
    for (int i = 0; i < info->num_vars; ++i) {
        //if (strcmp(ident, info->var_idents[i]) == 0) {
        if (strcmp(ident, info->var_infos[i].ident) == 0) {
            regid = i;
            break;
        }
    }

    if (regid == -1) {
        printf("internal error: could not find memory location of variable '%s'\n", ident);
        exit(1);
    }

    return regid * 4;
}

static int NewLabel() {
    int id = num_labels;
    num_labels += 1;
    return id;
}

static int CgX86LiteralInt(FileInfo *info, Literal *literal) {
    int regid = AllocRegister();
    fprintf(info->asmfile,
        "\tmov\t\t%s, %d\n",
        regs32[regid], literal->intvalue
    );

    return regid;
}

static int CgX86LiteralIdent(FileInfo *info, Literal *literal) {
    int regid = AllocRegister();
    int mem_location = FindMemLocation(info, literal->strvalue);
    fprintf(info->asmfile,
        "\tmov\t\t%s, [rsp+%d]\n",
        regs32[regid], mem_location
    );

    return regid;
}

static int CgX86LiteralPtr(FileInfo *info, Literal *literal) {
    int regid = AllocRegister();
    int mem_location = FindMemLocation(info, literal->strvalue);
    fprintf(info->asmfile,
        "\tlea\t\t%s, [rsp+%d]\n",
        regs64[regid], mem_location
    );

    return regid;
}

static int CgX86LiteralDeref(FileInfo *info, Literal *literal) {
    int regid1 = AllocRegister();
    int mem_location = FindMemLocation(info, literal->strvalue);
    fprintf(info->asmfile,
        "\tmov\t\t%s, [rsp+%d]\n"
        "\tmov\t\t%s, [%s]\n",
        regs64[regid1], mem_location,
        regs64[regid1], regs64[regid1]
    );

    FreeReg(regid1);
    return regid1;
}

static int CgX86BinaryOp(FileInfo *info, BinaryOp *binaryop, Bool is_jump, int label) {
    int regid_lhs = CgX86Expr(info, binaryop->lhs, FALSE, -1);
    int regid_rhs = CgX86Expr(info, binaryop->rhs, FALSE, -1);
    switch (binaryop->optype) {
        case OP_ADD: {
            fprintf(info->asmfile, "\tadd\t\t%s, %s\n", regs32[regid_lhs], regs32[regid_rhs]);
            FreeReg(regid_rhs);
            return regid_lhs;
        } break;
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
                    regs32[regid_lhs], regs32[regid_rhs],
                    inverse_branch_compares[binaryop->optype - OP_ISEQUAL], label
                );
            }
            else {
                fprintf(info->asmfile,
                    "\tcmp\t\t%s, %s\n"
                    "\t%s\t%s\n"
                    "\tand\t\t%s, 0xff\n",
                    regs32[regid_lhs], regs32[regid_rhs],
                    compares[binaryop->optype - OP_ISEQUAL], regs8[regid_lhs],
                    regs32[regid_lhs]
                );
            }
        } break;
        default: {
            printf("internal error: binary operator '%d' not implemented\n", binaryop->optype);
            exit(1);
        };
    }

    // TODO: Unsure why I must return 0 here and not some
    //       register in case OP_ISGREATER_THAN_EQUAL
    return 0;
}

static int CgX86Expr(FileInfo *info, Ast *expr, Bool is_jump, int label) {
    switch (expr->type) {
        case AST_LITERAL_INT:   return CgX86LiteralInt(info, (Literal *) expr);
        case AST_LITERAL_IDENT: return CgX86LiteralIdent(info, (Literal *) expr);
        case AST_LITERAL_PTR:   return CgX86LiteralPtr(info, (Literal *) expr);
        case AST_LITERAL_DEREF: return CgX86LiteralDeref(info, (Literal *) expr);
        case AST_BINARYOP:      return CgX86BinaryOp(info, (BinaryOp *) expr, is_jump, label);
        case AST_FUNCCALL: {
            CgX86FuncCall(info, (FuncCall *) expr);
            int regid = AllocRegister();
            fprintf(info->asmfile,
                "\tmov\t\t%s, eax\n",
                regs32[regid]
            );

            return regid;
        }
        default: {
            printf("internal error: invalid expression type '%d'\n", expr->type);
            exit(1);
        };
    }
}

static void CgX86VarAssign(FileInfo *info, VarAssign *varassign) {
    VarInfo varinfo = GetVarInfo(info, varassign->ident);
    int var_location = FindMemLocation(info, varassign->ident);
    int regid = CgX86Expr(info, varassign->expr, FALSE, -1);
    char *reg;
    if (varassign->is_deref) {
        switch (varinfo.datatype) {
            case DATA_CHAR_PTR: {reg = regs8[regid];} break;
            case DATA_INT_PTR:  {reg = regs32[regid];} break;
            default: {
                printf("internal error: invalid variable type1\n");
                exit(1);
            } break;
        }

        int regid2 = AllocRegister();
        fprintf(info->asmfile,
            "\tmov\t\t%s, [rsp+%d]\n"
            "\tmov\t\t[%s], %s\n",
            regs64[regid2], var_location,
            regs64[regid2], reg
        );
    }
    else {
        switch (varinfo.datatype) {
            case DATA_CHAR:    {reg = regs8[regid];} break;
            case DATA_INT:     {reg = regs32[regid];} break;
            case DATA_CHAR_PTR:
            case DATA_INT_PTR: {reg = regs64[regid];} break;
            default: {
                printf("internal error: invalid variable type2\n");
                exit(1);
            } break;
        }

        fprintf(info->asmfile,
            "\tmov\t\t[rsp+%d], %s\n",
            var_location, reg
        );
    }

    FreeRegs();
}

static void CgX86IfStmtElse(FileInfo *info, IfStmt *elsestmt, int end_label) {
    Bool has_elsestmt = elsestmt->elsestmt != 0;
    int false_label = (has_elsestmt) ? NewLabel() : end_label;
    if (elsestmt->condition != 0) {
        CgX86Expr(info, elsestmt->condition, TRUE, false_label);
        FreeRegs();
    }

    CgX86Block(info, elsestmt->block);
    if (has_elsestmt) {
        fprintf(info->asmfile,
            "\tjmp\t\tL%d\n"
            "L%d:\n",
            end_label, false_label
        );

        CgX86IfStmtElse(info, elsestmt->elsestmt, end_label);
    }
}

static void CgX86ReturnStmt(FileInfo *info, ReturnStmt *returnstmt) {
    int regid = CgX86Expr(info, returnstmt->expr, FALSE, -1);
    fprintf(info->asmfile,
        "\tmov\t\teax, %s\n",
        regs32[regid]
    );
}

static void CgX86IfStmt(FileInfo *info, IfStmt *ifstmt) {
    int false_label = NewLabel();
    int end_label = -1;
    CgX86Expr(info, ifstmt->condition, TRUE, false_label);
    FreeRegs();
    CgX86Block(info, ifstmt->block);
    if (ifstmt->elsestmt != 0) {
        end_label = NewLabel();
        fprintf(info->asmfile, "\tjmp\t\tL%d\n", end_label);
    }

    fprintf(info->asmfile, "L%d:\n", false_label);
    if (ifstmt->elsestmt != 0) {
        CgX86IfStmtElse(info, ifstmt->elsestmt, end_label);
        fprintf(info->asmfile, "L%d:\n", end_label);
    }
}

static void CgX86WhileLoop(FileInfo *info, WhileLoop *whileloop) {
    int start_label = NewLabel();
    int end_label = NewLabel();
    fprintf(info->asmfile, "L%d:\n", start_label);
    CgX86Expr(info, whileloop->condition, TRUE, end_label);
    FreeRegs();
    CgX86Block(info, whileloop->block);
    fprintf(info->asmfile,
        "\tjmp\t\tL%d\n"
        "L%d:\n",
        start_label, end_label
    );
}

static void CgX86FuncCall(FileInfo *info, FuncCall *funccall) {
    if (funccall->arg != 0) {
        int regid = CgX86Expr(info, funccall->arg, FALSE, -1);
        fprintf(info->asmfile, "\tmov\t\tecx, %s\n", regs32[regid]);
    }

    fprintf(info->asmfile, "\tcall\t%s\n", funccall->ident);
}

static void CgX86Block(FileInfo *info, Block *block) {
    Block *child_block = block;
    while (child_block != 0 && child_block->stmt != 0) {
        switch (child_block->stmt->type) {
            case AST_VARASSIGN:  {CgX86VarAssign(info, (VarAssign *) child_block->stmt);} break;
            case AST_RETURNSTMT: {CgX86ReturnStmt(info, (ReturnStmt *) child_block->stmt);} break;
            case AST_IFSTMT:     {CgX86IfStmt(info, (IfStmt *) child_block->stmt);} break;
            case AST_WHILELOOP:  {CgX86WhileLoop(info, (WhileLoop *) child_block->stmt);} break;
            case AST_FUNCCALL:   {CgX86FuncCall(info, (FuncCall *) child_block->stmt);} break;
            default: {
                printf("internal error: invalid statement type '%d'\n", child_block->stmt->type);
                exit(1);
            };
        }

        child_block = child_block->glue;
    }
}

static void CgX86FuncDecl(FileInfo *info, FuncDecl *funcdecl) {
    int align_bytes = 16 - (funcdecl->stack_depth_bytes % 16);
    if (align_bytes == 16) {
        align_bytes = 0;
    }

    // if function is called then bytes += 32 (shadow-space)
    int shadow_space = 32;
    int bytes = funcdecl->stack_depth_bytes + align_bytes + shadow_space + 8;
    fprintf(info->asmfile,
        "%s:\n"
        "\tsub\t\trsp, %d\n",
        funcdecl->ident,
        bytes
    );

    CgX86Block(info, funcdecl->block);
    FreeRegs();
    fprintf(info->asmfile,
        "\tadd\t\trsp, %d\n",
        bytes
    );

    fputs("\tret", info->asmfile);
}

void CgX86File(FileInfo *info, File *cfile) {
    fputs(
        "bits 64\n"
        "default rel\n"
        "\n"
        "segment .data\n"
        "\tfmt:\tdb \"%d\", 0xD, 0xA, 0x0\n"
        "\n"
        "segment .text\n"
        "\tglobal main\n"
        "\textern printf\n"
        "\n"
        "PrintInt:\n"
        "\tsub\t\trsp, 40\n"
        "\tmov\t\trdx, rcx\n"
        "\tlea\t\trcx, [fmt]\n"
        "\tcall\tprintf\n"
        "\tadd\t\trsp, 40\n"
        "\tret",
        info->asmfile
    );

    FreeRegs();
    File *child_file = cfile;
    while (child_file != 0 && child_file->funcdecl != 0) {
        fputs("\n\n", info->asmfile);
        CgX86FuncDecl(info, child_file->funcdecl);
        child_file = child_file->glue;
    }
}
