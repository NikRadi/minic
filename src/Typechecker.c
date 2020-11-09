#include "Typechecker.h"
#include "ErrorPrint.h"


static void TypecheckBlock(FileInfo *info, Block *block);
static void TypecheckExpr(FileInfo *info, Ast *expr);
static void TypecheckFuncCall(FileInfo *info, FuncCall *funccall);


static void TypecheckUnaryOp(FileInfo *info, UnaryOp *unaryop) {
    TypecheckExpr(info, unaryop->expr);
}

static void TypecheckBinaryOp(FileInfo *info, BinaryOp *binaryop) {
    TypecheckExpr(info, binaryop->lhs);
    TypecheckExpr(info, binaryop->rhs);
}

static void TypecheckExpr(FileInfo *info, Ast *expr) {
    switch (expr->type) {
        case AST_LITERAL_INT:
        case AST_LITERAL_CHAR:
        case AST_LITERAL_STR:
        case AST_LITERAL_PTR:
        case AST_LITERAL_IDENT: {} break;
        case AST_UNARYOP:       {TypecheckUnaryOp(info, (UnaryOp *) expr);} break;
        case AST_BINARYOP:      {TypecheckBinaryOp(info, (BinaryOp *) expr);} break;
        case AST_FUNCCALL:      {TypecheckFuncCall(info, (FuncCall *) expr);} break;
        default: {
            ThrowInternalError("operator '%d' not implemented", GetAstTypeStr(expr->type));
        };
    }
}

static void TypecheckVarDecl(FileInfo *info, VarDecl *vardecl, StorageType storagetype) {
    VarData vardata;
    vardata.datatype = vardecl->datatype;
    vardata.storagetype = storagetype;
    vardata.ident = vardecl->ident;
    vardata.lvl_indirection = vardecl->lvl_indirection;
    vardata.mem_location = info->current_scope->num_vars * 8;
    info->current_scope->vardatas[info->current_scope->num_vars] = vardata;
    info->current_scope->num_vars += 1;
    info->current_func->stack_depth_bytes += 8;
}

static void TypecheckVarAssign(FileInfo *info, BinaryOp *varassign) {
    TypecheckExpr(info, varassign->lhs);
    TypecheckExpr(info, varassign->rhs);
    if (ASOP_ADD_EQUAL <= varassign->optype && varassign->optype <= ASOP_DIV_EQUAL) {
        OperatorType optypes[4] = {BIOP_ADD, BIOP_SUB, BIOP_MUL, BIOP_DIV};
        BinaryOp *binaryop = NEW_AST(BinaryOp);
        binaryop->info.type = AST_BINARYOP;
        binaryop->optype = optypes[varassign->optype - ASOP_ADD_EQUAL];
        binaryop->rhs = varassign->rhs;

        Literal *literal = NEW_AST(Literal);
        literal->info.type = AST_LITERAL_IDENT;
        literal->strvalue = ((Literal *) varassign->lhs)->strvalue;
        binaryop->lhs = (Ast *) literal;

        varassign->optype = ASOP_EQUAL;
        varassign->rhs = (Ast *) binaryop;
    }
}

static void TypecheckReturnStmt(FileInfo *info, ReturnStmt *returnstmt) {
    TypecheckExpr(info, returnstmt->expr);
}

static void TypecheckIfStmt(FileInfo *info, IfStmt *ifstmt) {
    TypecheckExpr(info, ifstmt->condition);
    TypecheckBlock(info, ifstmt->block);
    if (ifstmt->elsestmt != NULL) {
        if (ifstmt->elsestmt->condition != NULL) {
            TypecheckIfStmt(info, ifstmt->elsestmt);
        }
        else {
            TypecheckBlock(info, ifstmt->elsestmt->block);
        }
    }
}

static void TypecheckWhileLoop(FileInfo *info, WhileLoop *whileloop) {
    TypecheckExpr(info, whileloop->condition);
    TypecheckBlock(info, whileloop->block);
}

static void TypecheckForLoop(FileInfo *info, ForLoop *forloop) {
    TypecheckExpr(info, (Ast *) forloop->pre_operation);
    TypecheckExpr(info, (Ast *) forloop->condition);
    TypecheckExpr(info, (Ast *) forloop->post_operation);
    TypecheckBlock(info, forloop->block);

    // WhileLoop *loop = NEW_AST(WhileLoop);
    // loop->info.type = AST_WHILELOOP;
    // loop->condition = (Ast *) forloop->condition;
    // loop->block = forloop->block;

    // // Insert forloop->post_operation as last statement in loop->block
    // Block *child_block = loop->block;
    // if (child_block->stmt == NULL) {
    //     child_block->stmt = (Ast *) forloop->post_operation;
    // }
    // else {
    //     while (TRUE) {
    //         Block *glue_postop = NEW_AST(Block);
    //         glue_postop->info.type = AST_BLOCK;
    //         glue_postop->stmt = (Ast *) forloop->post_operation;
    //         glue_postop->glue = NULL;
    //         if (child_block->glue == NULL) {
    //             child_block->glue = glue_postop;
    //             break;
    //         }
    //         else {
    //             child_block = child_block->glue;
    //         }
    //     }
    // }

    // ASSERT(forloop->info.parent->type == AST_BLOCK);
    // Block *parent_block = (Block *) forloop->info.parent;
    // Block *parent_glue = parent_block->glue;
    // parent_block->stmt = (Ast *) forloop->pre_operation;

    // Block *glue_loop = NEW_AST(Block);
    // glue_loop->info.type = AST_BLOCK;
    // glue_loop->stmt = (Ast *) loop;
    // parent_block->glue = glue_loop;
    // glue_loop->glue = parent_glue;
}

static void TypecheckFuncCall(FileInfo *info, FuncCall *funccall) {
    for (int i = 0; i < info->num_funcs; ++i) {
        FuncDecl *func = info->funcs[i];
        if (strcmp(func->ident, funccall->ident) == 0) {
            if (func->params.count != funccall->args.count) {
                printf("%s(%d) error: function call '%s' requires %d argument(s)\n",
                    info->filename, -1,
                    funccall->ident, func->params.count
                );
            }

            break;
        }
    }

    Node2Links *node = funccall->args.head;
    for (int i = 0; i < funccall->args.count; ++i) {
        TypecheckExpr(info, node->item);
        node = node->next;
    }
}

static void TypecheckBlock(FileInfo *info, Block *block) {
    Node2Links *node = block->stmts.head;
    if (node == NULL) {
        return;
    }

    while (TRUE) {
        switch (node->item->type) {
            case AST_VARDECL:    {TypecheckVarDecl(info, (VarDecl *) node->item, STOR_LOCAL);} break;
            case AST_BINARYOP:   {TypecheckVarAssign(info, (BinaryOp *) node->item);} break;
            case AST_RETURNSTMT: {TypecheckReturnStmt(info, (ReturnStmt *) node->item);} break;
            case AST_IFSTMT:     {TypecheckIfStmt(info, (IfStmt *) node->item);} break;
            case AST_WHILELOOP:  {TypecheckWhileLoop(info, (WhileLoop *) node->item);} break;
            case AST_FORLOOP:    {TypecheckForLoop(info, (ForLoop *) node->item);} break;
            case AST_FUNCCALL:   {TypecheckFuncCall(info, (FuncCall *) node->item);} break;
            default: {
                ThrowInternalError(
                    "statement '%s' not implemented in Typechecker",
                    GetAstTypeStr(node->item->type)
                );
            }
        }

        if (node == block->stmts.tail) {
            break;
        }

        node = node->next;
    }
}

static void TypecheckFuncDecl(FileInfo *info, FuncDecl *funcdecl) {
    for (int i = 0; i < info->num_funcs; ++i) {
        if (strcmp(funcdecl->ident, info->funcs[i]->ident) == 0) {
            printf("%s(%d) error: function '%s' already declared on line %d\n",
                info->filename, -1,
                funcdecl->ident, -1
            );
        }
    }

    funcdecl->block->scope.num_vars = 0;
    funcdecl->block->scope.parent = NULL;
    info->funcs[info->num_funcs] = funcdecl;
    info->num_funcs += 1;
    info->current_func = funcdecl;
    info->current_scope = &funcdecl->block->scope;

    Node2Links *node = funcdecl->params.head;
    for (int i = 0; i < funcdecl->params.count; ++i) {
        TypecheckVarDecl(info, (VarDecl *) node->item, STOR_PARAM);
        node = node->next;
    }

    TypecheckBlock(info, funcdecl->block);
    info->current_func = NULL;
    info->current_scope = NULL;
}

void TypecheckFile(FileInfo *info, File *file) {
    Node2Links *node = file->decls.head;
    if (node == NULL) {
        return;
    }

    for (int i = 0; i < file->decls.count; ++i) {
        switch (node->item->type) {
            case AST_FUNCDECL: {TypecheckFuncDecl(info, (FuncDecl *) node->item);} break;
            case AST_STRUCT:   {} break;
            default: {
                ThrowInternalError(
                    "typechecking not implemented for %s\n",
                    GetAstTypeStr(node->item->type)
                );
            }
        }

        node = node->next;
    }
}
