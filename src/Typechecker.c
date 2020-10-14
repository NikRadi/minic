#include "Typechecker.h"


static void TypecheckBlock(FileInfo *info, Block *block);
static void TypecheckExpr(FileInfo *info, Ast *expr);


static void TypecheckLiteral(FileInfo *info, Literal *literal) {

}

static void TypecheckBinaryOp(FileInfo *info, BinaryOp *binaryop) {
    TypecheckExpr(info, binaryop->lhs);
    TypecheckExpr(info, binaryop->rhs);
}

static void TypecheckExpr(FileInfo *info, Ast *expr) {
    switch (expr->type) {
        case AST_INT_LITERAL: {TypecheckLiteral(info, (Literal *) expr);} break;
        case AST_BINARYOP:    {TypecheckBinaryOp(info, (BinaryOp *) expr);} break;
        default: {
            printf("is this even possible\n");
            exit(1);
        };
    }
}

static void TypecheckVarDecl(FileInfo *info, VarDecl *vardecl) {
    info->current_func->stack_depth_bytes += 8;
    if (vardecl->expr != 0) {
        TypecheckExpr(info, vardecl->expr);
    }
}

static void TypecheckVarAssign(FileInfo *info, VarAssign *varassign) {
    TypecheckExpr(info, varassign->expr);
}

static void TypecheckIfStmt(FileInfo *info, IfStmt *ifstmt) {
    TypecheckExpr(info, ifstmt->condition);
    // TypecheckBlock(ifstmt->if_block);
}

static void TypecheckWhileLoop(FileInfo *info, WhileLoop *whileloop) {
    TypecheckExpr(info, whileloop->condition);
    TypecheckBlock(info, whileloop->block);
}

static void TypecheckForLoop(FileInfo *info, ForLoop *forloop) {
    TypecheckVarDecl(info, forloop->pre_operation);
    TypecheckExpr(info, forloop->condition);
    TypecheckVarAssign(info, forloop->post_operation);
    TypecheckBlock(info, forloop->block);
}

static void TypecheckBlock(FileInfo *info, Block *block) {
    Block *child_block = block;
    while (child_block != 0 && child_block->stmt != 0) {
        switch (child_block->stmt->type) {
            case AST_VARDECL:   {TypecheckVarDecl(info, (VarDecl *) child_block->stmt);} break;
            case AST_VARASSIGN: {TypecheckVarAssign(info, (VarAssign *)child_block->stmt);} break;
            case AST_IFSTMT:    {TypecheckIfStmt(info, (IfStmt *) child_block->stmt);} break;
            case AST_WHILELOOP: {TypecheckWhileLoop(info, (WhileLoop *) child_block->stmt);} break;
            case AST_FORLOOP:   {TypecheckForLoop(info, (ForLoop *) child_block->stmt);} break;
        }

        child_block = child_block->glue;
    }
}

static void TypecheckFuncDecl(FileInfo *info, FuncDecl *funcdecl) {
    info->current_func = funcdecl;
    TypecheckBlock(info, funcdecl->block);
}

void TypecheckFile(FileInfo *info, File *file) {
    File *child_file = file;
    while (child_file != 0 && child_file->funcdecl != 0) {
        TypecheckFuncDecl(info, child_file->funcdecl);
        child_file = child_file->glue;
    }
}
