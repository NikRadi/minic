#include "Typechecker.h"


static void TypecheckBlock(FileInfo *info, Block *block);
static void TypecheckExpr(FileInfo *info, Ast *expr);
static void TypecheckFuncCall(FileInfo *info, FuncCall *funccall);


static void TypecheckLiteral(FileInfo *info, Literal *literal) {
}

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
        case AST_LITERAL_IDENT: {TypecheckLiteral(info, (Literal *) expr);} break;
        case AST_UNARYOP:       {TypecheckUnaryOp(info, (UnaryOp *) expr);} break;
        case AST_BINARYOP:      {TypecheckBinaryOp(info, (BinaryOp *) expr);} break;
        case AST_FUNCCALL:      {TypecheckFuncCall(info, (FuncCall *) expr);} break;
        default: {
            printf("internal error: unimplemented expr case '%s'\n", GetAstTypeStr(expr->type));
            exit(1);
        };
    }
}

static void TypecheckVarDecl(FileInfo *info, VarDecl *vardecl) {
    if (vardecl->expr != NULL) {
        TypecheckExpr(info, vardecl->expr);
        BinaryOp *varassign = NEW_AST(BinaryOp);
        varassign->info.type = AST_BINARYOP;
        varassign->rhs = vardecl->expr;

        Literal *literal = NEW_AST(Literal);
        literal->info.type = AST_LITERAL_IDENT;
        literal->strvalue = vardecl->ident;
        varassign->lhs = (Ast *) literal;

        ASSERT(vardecl->info.parent->type == AST_BLOCK);
        Block *parent_block = (Block *) vardecl->info.parent;
        parent_block->stmt = (Ast *) varassign;
    }

    VarInfo varinfo;
    varinfo.ident = vardecl->ident;
    varinfo.datatype = vardecl->datatype;
    info->var_infos[info->num_vars] = varinfo;
    info->num_vars += 1;
    info->current_func->stack_depth_bytes += 4;
}

static void TypecheckVarAssign(FileInfo *info, BinaryOp *varassign) {

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

    WhileLoop *loop = NEW_AST(WhileLoop);
    loop->info.type = AST_WHILELOOP;
    loop->condition = (Ast *) forloop->condition;
    loop->block = forloop->block;

    // Insert forloop->post_operation as last statement in loop->block
    Block *child_block = loop->block;
    if (child_block->stmt == NULL) {
        child_block->stmt = (Ast *) forloop->post_operation;
    }
    else {
        while (TRUE) {
            Block *glue_postop = NEW_AST(Block);
            glue_postop->info.type = AST_BLOCK;
            glue_postop->stmt = (Ast *) forloop->post_operation;
            glue_postop->glue = NULL;
            if (child_block->glue == NULL) {
                child_block->glue = glue_postop;
                break;
            }
            else {
                child_block = child_block->glue;
            }
        }
    }

    ASSERT(forloop->info.parent->type == AST_BLOCK);
    Block *parent_block = (Block *) forloop->info.parent;
    Block *parent_glue = parent_block->glue;
    parent_block->stmt = (Ast *) forloop->pre_operation;

    Block *glue_loop = NEW_AST(Block);
    glue_loop->info.type = AST_BLOCK;
    glue_loop->stmt = (Ast *) loop;
    parent_block->glue = glue_loop;
    glue_loop->glue = parent_glue;
}

static void TypecheckFuncCall(FileInfo *info, FuncCall *funccall) {
    if (funccall->arg != NULL) {
        TypecheckExpr(info, funccall->arg);
    }
}

static void TypecheckBlock(FileInfo *info, Block *block) {
    Block *child_block = block;
    while (child_block != NULL && child_block->stmt != NULL) {
        switch (child_block->stmt->type) {
            case AST_VARDECL:    {TypecheckVarDecl(info, (VarDecl *) child_block->stmt);} break;
            case AST_BINARYOP:   {TypecheckVarAssign(info, (BinaryOp *) child_block->stmt);} break;
            case AST_RETURNSTMT: {TypecheckReturnStmt(info, (ReturnStmt *) child_block->stmt);} break;
            case AST_IFSTMT:     {TypecheckIfStmt(info, (IfStmt *) child_block->stmt);} break;
            case AST_WHILELOOP:  {TypecheckWhileLoop(info, (WhileLoop *) child_block->stmt);} break;
            case AST_FORLOOP:    {TypecheckForLoop(info, (ForLoop *) child_block->stmt);} break;
            case AST_FUNCCALL:   {TypecheckFuncCall(info, (FuncCall *) child_block->stmt);} break;
            default: {
                printf("invalid statement '%s'\n", GetAstTypeStr(child_block->stmt->type));
                exit(1);
            };
        }

        child_block = child_block->glue;
    }
}

static void TypecheckFuncDecl(FileInfo *info, FuncDecl *funcdecl) {
    info->func_idents[info->num_funcs] = funcdecl->ident;
    info->num_funcs += 1;

    info->current_func = funcdecl;
    TypecheckBlock(info, funcdecl->block);
}

void TypecheckFile(FileInfo *info, File *file) {
    File *child_file = file;
    while (child_file != NULL && child_file->funcdecl != NULL) {
        TypecheckFuncDecl(info, child_file->funcdecl);
        child_file = child_file->glue;
    }
}
