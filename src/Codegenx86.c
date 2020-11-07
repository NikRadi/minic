#include "CodegenX86.h"
#include "ErrorPrint.h"


static void CgX86Block(FileInfo *info, Block *block);
static int CgX86Expr(FileInfo *info, Ast *expr, Bool is_jump, int label);
static void CgX86FuncCall(FileInfo *info, FuncCall *funccall);


static Bool is_reg_free[4];
static char *regs64[] = {"r8", "r9", "r10", "r11"};
static char *regs32[] = {"r8d", "r9d", "r10d", "r11d"};
static char *regs8[] = {"r8b", "r9b", "r10b", "r11b"};
static char *compares_set[] = {"sete", "setne", "setl", "setle", "setg", "setge"};
static char *compares_jmp_inverse[] = {"jne", "je", "jge", "jg", "jle", "je"};
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

    ThrowInternalError("out of registers");
    return -1; // To get rid of warning C4715
}

static VarData GetVarData(FileInfo *info, char *ident) {
    for (int i = 0; i < info->current_scope->num_vars; ++i) {
        if (strcmp(ident, info->current_scope->vardatas[i].ident) == 0) {
            return info->current_scope->vardatas[i];
        }
    }

    ThrowInternalError("could not find data of variable '%s'", ident);
    return info->current_scope->vardatas[0]; // To get rid of warning C4715
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
    VarData vardata = GetVarData(info, literal->strvalue);
    char *reg;
    if (vardata.lvl_indirection > 0) {
        reg = regs64[regid];
    }
    else {
        switch (vardata.datatype) {
            case DATA_CHAR:     {reg = regs8[regid];} break;
            case DATA_INT:      {reg = regs32[regid];} break;
            default: {
                ThrowInternalError("variable datatype '%d' not implemented", vardata.datatype);
                reg = 0; // To get rid of warning C4701
            }
        }
    }

    fprintf(info->asmfile, "\tmov\t\t%s, [rsp+%d]\n", reg, vardata.mem_location);
    return regid;
}

static int CgX86LiteralPtr(FileInfo *info, Literal *literal) {
    int regid = AllocRegister();
    VarData vardata = GetVarData(info, literal->strvalue);
    fprintf(info->asmfile,
        "\tlea\t\t%s, [rsp+%d]\n",
        regs64[regid], vardata.mem_location
    );

    return regid;
}

static int CgX86LiteralDeref(FileInfo *info, Literal *literal) {
    int regid = AllocRegister();
    VarData vardata = GetVarData(info, literal->strvalue);
    fprintf(info->asmfile,
        "\tmov\t\t%s, [rsp+%d]\n"
        "\tmov\t\t%s, [%s]\n",
        regs64[regid], vardata.mem_location,
        regs64[regid], regs64[regid]
    );

    return regid;
}

static int CgX86UnaryOp(FileInfo *info, UnaryOp *unaryop) {
    switch (unaryop->optype) {
        case UNOP_ADDRESS: {
            ASSERT(unaryop->expr->type == AST_LITERAL_IDENT);
            int regid = AllocRegister();
            Literal *literal = (Literal *) unaryop->expr;
            VarData vardata = GetVarData(info, literal->strvalue);
            fprintf(info->asmfile,
                "\tlea\t\t%s, [rsp+%d]\n",
                regs64[regid], vardata.mem_location
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
        case UNOP_NOT: {
            int regid = CgX86Expr(info, unaryop->expr, FALSE, -1);
            fprintf(info->asmfile,
                "\ttest\t%s, %s\n"
                "\tsete\t%s\n",
                regs64[regid], regs64[regid],
                regs8[regid]
            );

            return regid;
        }
        default: {
            ThrowInternalError("unaryop operator '%d' not implemented", unaryop->optype);
            return -1; // To get rid of warning C4715
        }
    }
}

static int CgX86BinaryOp(FileInfo *info, BinaryOp *binaryop, Bool is_jump, int label) {
    if (binaryop->optype == BIOP_ARR_IDX) {
        ASSERT(binaryop->lhs->type == AST_LITERAL_IDENT);
        Literal *literal = (Literal *) binaryop->lhs;
        VarData vardata = GetVarData(info, literal->strvalue);
        int regid_rhs = CgX86Expr(info, binaryop->rhs, FALSE, -1);
        fprintf(info->asmfile,
            "\timul\t%s, %s, 8\n"
            "\tmov\t\t%s, [rsp+%d+%s]\n",
            regs64[regid_rhs], regs64[regid_rhs],
            regs64[regid_rhs], vardata.mem_location, regs64[regid_rhs]
        );

        return regid_rhs;
    }

    int regid_lhs = CgX86Expr(info, binaryop->lhs, FALSE, -1);
    int regid_rhs = CgX86Expr(info, binaryop->rhs, FALSE, -1);
    switch (binaryop->optype) {
        case BIOP_AND:
        case BIOP_OR: {
            char *compares[2] = {"and", "or"};
            fprintf(info->asmfile,
                "\t%s\t\t%s, %s\n",
                compares[binaryop->optype - BIOP_AND],
                regs64[regid_lhs], regs64[regid_rhs]
            );

            FreeReg(regid_rhs);
            return regid_lhs;
        } break;
        case BIOP_ISEQUAL:
        case BIOP_NOTEQUAL:
        case BIOP_LESS:
        case BIOP_LESS_EQUAL:
        case BIOP_GREATER:
        case BIOP_GREATER_EQUAL: {
            int compares_idx = binaryop->optype - BIOP_ISEQUAL;
            ASSERT(0 <= compares_idx  && compares_idx <= 6);
            if (is_jump) {
                fprintf(info->asmfile,
                    "\tcmp\t\t%s, %s\n"
                    "\t%s\t\tL%d\n",
                    regs32[regid_lhs], regs32[regid_rhs],
                    compares_jmp_inverse[compares_idx], label
                );

                // TODO: What does the caller use this value for?
                return -1;
            }
            else {
                fprintf(info->asmfile,
                    "\tcmp\t\t%s, %s\n"
                    "\t%s\t%s\n"
                    "\tand\t\t%s, 0xff\n",
                    regs32[regid_lhs], regs32[regid_rhs],
                    compares_set[compares_idx], regs8[regid_lhs],
                    regs32[regid_lhs]
                );

                FreeReg(regid_rhs);
                return regid_lhs;
            }
        } break;
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
        default: {
            ThrowInternalError("binary operator '%ld' is not implemented\n", binaryop->optype);
            return -1; // To get rid of warning C4715
        };
    }
}

static int CgX86Expr(FileInfo *info, Ast *expr, Bool is_jump, int label) {
    switch (expr->type) {
        case AST_LITERAL_INT:
        case AST_LITERAL_CHAR:  return CgX86LiteralInt(info, (Literal *) expr);
        case AST_LITERAL_STR:   ASSERT(FALSE); // Not implemented
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
            ThrowInternalError("invalid expression type '%s'", GetAstTypeStr(expr->type));
            return -1; // To get rid of warning C4715
        };
    }
}

static void CgX86VarDecl(FileInfo *info, VarDecl *vardecl) {
    if (vardecl->expr != NULL) {
        int regid = CgX86Expr(info, vardecl->expr, FALSE, -1);
        VarData vardata = GetVarData(info, vardecl->ident);
        fprintf(info->asmfile,
            "\tmov\t\t[rsp+%d], %s\n",
            vardata.mem_location, regs64[regid]
        );
    }
}

static void CgX86VarAssign(FileInfo *info, BinaryOp *varassign) {
    int regid = CgX86Expr(info, varassign->rhs, FALSE, -1);
    switch (varassign->lhs->type) {
        case AST_LITERAL_IDENT: {
            Literal *literal = (Literal *) varassign->lhs;
            VarData vardata = GetVarData(info, literal->strvalue);
            fprintf(info->asmfile,
                "\tmov\t\t[rsp+%d], %s\n",
                vardata.mem_location, regs64[regid]
            );

            FreeReg(regid);
        } break;
        case AST_UNARYOP: {
            UnaryOp *unaryop = (UnaryOp *) varassign->lhs;
            if (unaryop->optype == UNOP_DEREF) {
                int regid2;
                if (unaryop->expr->type == AST_LITERAL_IDENT) {
                    Literal *literal = (Literal *) unaryop->expr;
                    VarData vardata = GetVarData(info, literal->strvalue);
                    regid2 = AllocRegister();
                    fprintf(info->asmfile,
                        "\tmov\t\t%s, [rsp+%d]\n",
                        regs64[regid2], vardata.mem_location
                    );
                }
                else {
                    regid2 = CgX86Expr(info, unaryop->expr, FALSE, -1);
                }

                fprintf(info->asmfile,
                    "\tmov\t\t[%s], %s\n",
                    regs64[regid2], regs32[regid]
                );

                FreeReg(regid);
                FreeReg(regid2);
            }
            else {
                ThrowInternalError("assignment unary operator l-value '%d' not implemented",
                    unaryop->optype
                );
            }
        } break;
        case AST_BINARYOP: {
            BinaryOp *binaryop = (BinaryOp *) varassign->lhs;
            ASSERT(binaryop->optype == BIOP_ARR_IDX);
            Literal *literal = (Literal *) binaryop->lhs;
            VarData vardata = GetVarData(info, literal->strvalue);
            int regid_rhs = CgX86Expr(info, binaryop->rhs, FALSE, -1);
            fprintf(info->asmfile,
                "\timul\t%s, %s, 8\n"
                "\tmov\t\t[rsp+%d+%s], %s\n",
                regs64[regid_rhs], regs64[regid_rhs],
                vardata.mem_location, regs64[regid_rhs], regs64[regid]
            );

            FreeReg(regid);
            FreeReg(regid_rhs);
        } break;
        default: {
            ThrowInternalError(
                "assignment l-value '%s' not implemented",
                GetAstTypeStr(varassign->lhs->type)
            );
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
    ASSERT(funccall->args.count <= 2);
    Node2Links *node = funccall->args.head;
    char *regs[] = {"rcx", "rdx"};
    for (int i = 0; i < funccall->args.count; ++i) {
        int regid = CgX86Expr(info, node->item, FALSE, -1);
        fprintf(info->asmfile, "\tmov\t\t%s, %s\n", regs[i], regs64[regid]);
        node = node->next;
    }

    fprintf(info->asmfile, "\tcall\t%s\n", funccall->ident);
}

static void CgX86Block(FileInfo *info, Block *block) {
    Node2Links *node = block->stmts.head;
    if (node == NULL) {
        return;
    }

    while (TRUE) {
        switch (node->item->type) {
            case AST_VARDECL:    {CgX86VarDecl(info, (VarDecl *) node->item);} break;
            case AST_BINARYOP:   {CgX86VarAssign(info, (BinaryOp *) node->item);} break;
            case AST_RETURNSTMT: {CgX86ReturnStmt(info, (ReturnStmt *) node->item);} break;
            case AST_IFSTMT:     {CgX86IfStmt(info, (IfStmt *) node->item);} break;
            case AST_WHILELOOP:  {CgX86WhileLoop(info, (WhileLoop *) node->item);} break;
            case AST_FUNCCALL:   {CgX86FuncCall(info, (FuncCall *) node->item);} break;
            default: {
                ThrowInternalError(
                    "statement '%s' not implemented in CodegenX86",
                    GetAstTypeStr(node->item->type)
                );
            }
        }

        // This statement fails TestCharOverflow for some reason
        FreeRegs();
        if (node == block->stmts.tail) {
            break;
        }

        node = node->next;
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

    char *regs[] = {"rcx", "rdx"};
    for (int i = 0; i < funcdecl->block->scope.num_vars; ++i) {
        VarData vardata = funcdecl->block->scope.vardatas[i];
        if (vardata.storagetype == STOR_PARAM) {
            fprintf(info->asmfile, "\tmov\t\t[rsp+%d], %s\n", i * 8, regs[i]);
        }
    }

    info->current_func = funcdecl;
    info->current_scope = &funcdecl->block->scope;
    CgX86Block(info, funcdecl->block);
    info->current_func = NULL;
    info->current_scope = NULL;
    FreeRegs();
    fprintf(info->asmfile,
        "\tadd\t\trsp, %d\n",
        bytes
    );

    fputs("\tret", info->asmfile);
}

void CgX86File(FileInfo *info, File *file) {
    Node2Links *node = file->funcdecls.head;
    if (node == NULL) {
        return;
    }

    fputs(
        "bits 64\n"
        "default rel\n"
        "\n"
        "segment .data\n"
        "\tifmt:\tdb \"%d\", 0xD, 0xA, 0x0\n"
        "\tcfmt:\tdb \"%c\", 0xD, 0xA, 0x0\n"
        "\n"
        "segment .text\n"
        "\tglobal main\n"
        "\textern printf\n"
        "\n"
        "PrintInt:\n"
        "\tsub\t\trsp, 40\n"
        "\tmov\t\trdx, rcx\n"
        "\tlea\t\trcx, [ifmt]\n"
        "\tcall\tprintf\n"
        "\tadd\t\trsp, 40\n"
        "\tret\n"
        "\n"
        "PrintChar:\n"
        "\tsub\t\trsp, 40\n"
        "\tmov\t\trdx, rcx\n"
        "\tlea\t\trcx, [cfmt]\n"
        "\tcall\tprintf\n"
        "\tadd\t\trsp, 40\n"
        "\tret",
        info->asmfile
    );

    FreeRegs();
    while (TRUE) {
        fputs("\n\n", info->asmfile);
        CgX86FuncDecl(info, (FuncDecl *) node->item);
        if (node == file->funcdecls.tail) {
            break;
        }

        node = node->next;
    }
}
