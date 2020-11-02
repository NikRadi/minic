#include "CodegenX86.h"


static void CgX86Block(FileInfo *info, Block *block);
static int CgX86Expr(FileInfo *info, Ast *expr, Bool is_jump, int label);
static void CgX86FuncCall(FileInfo *info, FuncCall *funccall);


static Bool is_reg_free[4];
static char *regs64[] = {"r8", "r9", "r10", "r11"};
static char *regs32[] = {"r8d", "r9d", "r10d", "r11d"};
static char *regs8[] = {"r8b", "r9b", "r10b", "r11b"};
// Index 0 is NULL because OP_ISINVALID is value 0 and BIOP_ISEQUAL is value 1
static char *compares_set[] = {NULL, "sete", "setne", "setl", "setle", "setg", "setge"};
static char *compares_inverse_jump[] = {NULL, "jne", "je", "jge", "jg", "jle", "je"};
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
        if (strcmp(ident, info->var_infos[i].ident) == 0) {
            regid = i;
            break;
        }
    }

    if (regid == -1) {
        printf("internal error: could not find memory location of variable '%s'\n", ident);
        exit(1);
    }

    return regid * 8;
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
        regs64[regid], literal->intvalue
    );

    return regid;
}

static int CgX86LiteralIdent(FileInfo *info, Literal *literal) {
    int regid = AllocRegister();
    int mem_location = FindMemLocation(info, literal->strvalue);
    if (literal->arridx == -1) {
        VarInfo varinfo = GetVarInfo(info, literal->strvalue);
        char *reg;
        switch (varinfo.datatype) {
            case DATA_CHAR:    {reg = regs8[regid];} break;
            case DATA_INT:     {reg = regs32[regid];} break;
            case DATA_CHAR_PTR:
            case DATA_INT_PTR: {reg = regs64[regid];} break;
            default: {
                printf("internal error: not implemented\n");
                exit(1);
            }
        }

        fprintf(info->asmfile,
            "\tmov\t\t%s, [rsp+%d]\n",
            reg, mem_location
        );
    }
    else {
        fprintf(info->asmfile,
            "\tmov\t\t%s, 8\n"
            "\timul\t%s, %s, %d\n"
            "\tmov\t\t%s, [rsp+%d+%s]\n",
            regs64[regid],
            regs64[regid], regs64[regid], literal->arridx,
            regs64[regid], mem_location, regs64[regid]
        );
    }

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

    return regid1;
}

static int CgX86UnaryOp(FileInfo *info, UnaryOp *unaryop) {
    switch (unaryop->optype) {
        case UNOP_ADDRESS: {
            ASSERT(unaryop->expr->type == AST_LITERAL_IDENT);
            int regid = AllocRegister();
            Literal *literal = (Literal *) unaryop->expr;
            int mem_location = FindMemLocation(info, literal->strvalue);
            fprintf(info->asmfile,
                "\tlea\t\t%s, [rsp+%d]\n",
                regs64[regid], mem_location
            );

            return regid;
        };
        case UNOP_DEREF: {
            int regid = CgX86Expr(info, unaryop->expr, FALSE, -1);
            fprintf(info->asmfile,
                "\tmov\t\t%s, [%s]\n",
                regs64[regid], regs64[regid]
            );

            return regid;
        };
        default: {
            printf("err\n");
            exit(1);
        }
    }
}

static int CgX86BinaryOp(FileInfo *info, BinaryOp *binaryop, Bool is_jump, int label) {
    int regid_lhs = CgX86Expr(info, binaryop->lhs, FALSE, -1);
    int regid_rhs = CgX86Expr(info, binaryop->rhs, FALSE, -1);
    switch (binaryop->optype) {
        case BIOP_ADD:
        case BIOP_SUB:
        case BIOP_MUL: {
            char *optypes[3] = {"add\t", "sub\t", "imul"};
            fprintf(info->asmfile,
                "\t%s\t%s, %s\n",
                optypes[binaryop->optype - BIOP_ADD], regs64[regid_lhs], regs64[regid_rhs]
            );

            FreeReg(regid_rhs);
            return regid_lhs;
        }
        case BIOP_ISEQUAL:
        case BIOP_NOTEQUAL:
        case BIOP_LESS:
        case BIOP_LESS_EQUAL:
        case BIOP_GREATER:
        case BIOP_GREATER_EQUAL: {
            if (is_jump) {
                fprintf(info->asmfile,
                    "\tcmp\t\t%s, %s\n"
                    "\t%s\t\tL%d\n",
                    regs32[regid_lhs], regs32[regid_rhs],
                    compares_inverse_jump[binaryop->optype], label
                );
            }
            else {
                fprintf(info->asmfile,
                    "\tcmp\t\t%s, %s\n"
                    "\t%s\t%s\n"
                    "\tand\t\t%s, 0xff\n",
                    regs32[regid_lhs], regs32[regid_rhs],
                    compares_set[binaryop->optype], regs8[regid_lhs],
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
        case AST_UNARYOP:       return CgX86UnaryOp(info, (UnaryOp *) expr);
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
            printf("internal error: invalid expression type '%s'\n", GetAstTypeStr(expr->type));
            exit(1);
        };
    }
}

static void CgX86VarAssign(FileInfo *info, BinaryOp *varassign) {
    int regid = CgX86Expr(info, varassign->rhs, FALSE, -1);
    switch (varassign->lhs->type) {
        case AST_LITERAL_IDENT: {
            Literal *literal = (Literal *) varassign->lhs;
            int mem_location = FindMemLocation(info, literal->strvalue);
            if (literal->arridx == -1) {
                fprintf(info->asmfile,
                    "\tmov\t\t[rsp+%d], %s\n",
                    mem_location, regs64[regid]
                );
            }
            else {
                int regid2 = AllocRegister();
                fprintf(info->asmfile,
                    "\tmov\t\t%s, 8\n"
                    "\timul\t%s, %s, %d\n"
                    "\tmov\t\t[rsp+%d+%s], %s\n",
                    regs64[regid2],
                    regs64[regid2], regs64[regid2], literal->arridx,
                    mem_location, regs64[regid2], regs64[regid]
                );

                FreeReg(regid2);
            }
        } break;
        case AST_UNARYOP: {
            UnaryOp *unaryop = (UnaryOp *) varassign->lhs;
            switch (unaryop->optype) {
                case UNOP_DEREF: {
                    int regid2;
                    if (unaryop->expr->type == AST_LITERAL_IDENT) {
                        Literal *literal = (Literal *) unaryop->expr;
                        int mem_location = FindMemLocation(info, literal->strvalue);
                        regid2 = AllocRegister();
                        fprintf(info->asmfile,
                            "\tmov\t\t%s, [rsp+%d]\n",
                            regs64[regid2], mem_location
                        );
                    }
                    else {
                        regid2 = CgX86Expr(info, unaryop->expr, FALSE, -1);
                    }

                    fprintf(info->asmfile,
                        "\tmov\t\t[%s], %s\n",
                        regs64[regid2], regs32[regid]
                    );
                } break;
                default: {
                    printf("internal error: still not implemented op '%d'\n", unaryop->optype);
                    exit(1);
                }
            }
        } break;
        default: {
            printf("internal error: not implemented '%s'\n", GetAstTypeStr(varassign->lhs->type));
            exit(1);
        }
    }
}

static void CgX86IfStmtElse(FileInfo *info, IfStmt *elsestmt, int end_label) {
    Bool has_elsestmt = elsestmt->elsestmt != NULL;
    int false_label = (has_elsestmt) ? NewLabel() : end_label;
    if (elsestmt->condition != NULL) {
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
    if (ifstmt->elsestmt != NULL) {
        end_label = NewLabel();
        fprintf(info->asmfile, "\tjmp\t\tL%d\n", end_label);
    }

    fprintf(info->asmfile, "L%d:\n", false_label);
    if (ifstmt->elsestmt != NULL) {
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
    if (funccall->arg != NULL) {
        int regid = CgX86Expr(info, funccall->arg, FALSE, -1);
        fprintf(info->asmfile, "\tmov\t\tecx, %s\n", regs32[regid]);
    }

    fprintf(info->asmfile, "\tcall\t%s\n", funccall->ident);
}

static void CgX86Block(FileInfo *info, Block *block) {
    Block *child_block = block;
    while (child_block != NULL && child_block->stmt != NULL) {
        switch (child_block->stmt->type) {
            case AST_VARDECL:    {} break;
            case AST_BINARYOP:   {CgX86VarAssign(info, (BinaryOp *) child_block->stmt);} break;
            case AST_RETURNSTMT: {CgX86ReturnStmt(info, (ReturnStmt *) child_block->stmt);} break;
            case AST_IFSTMT:     {CgX86IfStmt(info, (IfStmt *) child_block->stmt);} break;
            case AST_WHILELOOP:  {CgX86WhileLoop(info, (WhileLoop *) child_block->stmt);} break;
            case AST_FUNCCALL:   {CgX86FuncCall(info, (FuncCall *) child_block->stmt);} break;
            default: {
                ASSERT(child_block != NULL);
                printf("internal error: invalid statement type '%s'\n", GetAstTypeStr(child_block->stmt->type));
                exit(1);
            };
        }

        child_block = child_block->glue;
        FreeRegs();
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
    while (child_file != NULL && child_file->funcdecl != NULL) {
        fputs("\n\n", info->asmfile);
        CgX86FuncDecl(info, child_file->funcdecl);
        child_file = child_file->glue;
    }
}
