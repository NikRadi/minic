#include "Typechecker.h"


static void TypecheckBlock(FileInfo *info, Block *block);
static void TypecheckExpr(FileInfo *info, Ast *expr);
static void TypecheckFuncCall(FileInfo *info, FuncCall *funccall);


static void TypecheckLiteral(FileInfo *info, Literal *literal) {

}

static void TypecheckBinaryOp(FileInfo *info, BinaryOp *binaryop) {
    TypecheckExpr(info, binaryop->lhs);
    TypecheckExpr(info, binaryop->rhs);
}

static void TypecheckExpr(FileInfo *info, Ast *expr) {
    switch (expr->type) {
        case AST_LITERAL_INT:
        case AST_LITERAL_IDENT:
        case AST_LITERAL_PTR:
        case AST_LITERAL_DEREF: {TypecheckLiteral(info, (Literal *) expr);} break;
        case AST_BINARYOP:      {TypecheckBinaryOp(info, (BinaryOp *) expr);} break;
        case AST_FUNCCALL:      {TypecheckFuncCall(info, (FuncCall *) expr);} break;
        default: {
            printf("is this even possible\n");
            exit(1);
        };
    }
}

static void TypecheckVarDecl(FileInfo *info, VarDecl *vardecl) {
    VarInfo varinfo;
    varinfo.ident = vardecl->ident;
    varinfo.datatype = vardecl->datatype;
    info->var_infos[info->num_vars] = varinfo;

    info->num_vars += 1;
    info->current_func->stack_depth_bytes += 4;
    ASSERT(vardecl->info.parent->type == AST_BLOCK);
    Block *parent_block = (Block *) vardecl->info.parent;
    if (vardecl->expr != 0) {
        TypecheckExpr(info, vardecl->expr);
        VarAssign *varassign = NEW_AST(VarAssign);
        varassign->info.type = AST_VARASSIGN;
        varassign->info.parent = vardecl->info.parent;
        varassign->ident = vardecl->ident;
        varassign->expr = vardecl->expr;
        parent_block->stmt = (Ast *) varassign;
    }
    else {
        ASSERT(parent_block->info.parent != 0);
        if (parent_block->glue != 0) {
            switch (parent_block->info.parent->type) {
                case AST_FUNCDECL: {
                    FuncDecl *funcdecl = (FuncDecl *) parent_block->info.parent;
                    funcdecl->block = parent_block->glue;
                    funcdecl->block->info.parent = (Ast *) funcdecl;
                } break;
                case AST_BLOCK: {
                    Block *block = (Block *) parent_block->info.parent;
                    block->glue = parent_block->glue;
                    block->glue->info.parent = (Ast *) block;
                } break;
                default: {
                    printf("internal error: not implemented1 %d\n",
                        parent_block->info.parent->type
                    );
                    exit(1);
                }
            }
        }
        else {
            switch (parent_block->info.parent->type) {
                case AST_FUNCDECL: {
                    FuncDecl *funcdecl = (FuncDecl *) parent_block->info.parent;
                    funcdecl->block = 0;
                } break;
                case AST_BLOCK: {
                    Block *block = (Block *) parent_block->info.parent;
                    block->glue = 0;
                } break;
                default: {
                    printf("internal error: not implemented2 %d, %s\n",
                        parent_block->info.parent->type,
                        vardecl->ident
                    );
                    exit(1);
                }
            }
        }
    }
}

static void TypecheckVarAssign(FileInfo *info, VarAssign *varassign) {
    TypecheckExpr(info, varassign->expr);
}

static void TypecheckReturnStmt(FileInfo *info, ReturnStmt *returnstmt) {
    TypecheckExpr(info, returnstmt->expr);
}

static void TypecheckIfStmt(FileInfo *info, IfStmt *ifstmt) {
    TypecheckExpr(info, ifstmt->condition);
    TypecheckBlock(info, ifstmt->block);
    if (ifstmt->elsestmt != 0) {
        if (ifstmt->elsestmt->condition != 0) {
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
    TypecheckVarAssign(info, forloop->pre_operation);
    TypecheckExpr(info, forloop->condition);
    TypecheckVarAssign(info, forloop->post_operation);
    TypecheckBlock(info, forloop->block);

    WhileLoop *loop = NEW_AST(WhileLoop);
    loop->info.type = AST_WHILELOOP;
    loop->condition = forloop->condition;
    loop->block = forloop->block;

    // Insert forloop->post_operation as last statement in loop->block
    Block *child_block = loop->block;
    if (child_block->stmt == 0) {
        child_block->stmt = (Ast *) forloop->post_operation;
    }
    else {
        while (TRUE) {
            Block *glue_postop = NEW_AST(Block);
            glue_postop->info.type = AST_BLOCK;
            glue_postop->stmt = (Ast *) forloop->post_operation;
            glue_postop->glue = 0;
            if (child_block->glue == 0) {
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
    if (funccall->arg != 0) {
        TypecheckExpr(info, funccall->arg);
    }
}

static void TypecheckBlock(FileInfo *info, Block *block) {
    Block *child_block = block;
    while (child_block != 0 && child_block->stmt != 0) {
        switch (child_block->stmt->type) {
            case AST_VARDECL:    {TypecheckVarDecl(info, (VarDecl *) child_block->stmt);} break;
            case AST_VARASSIGN:  {TypecheckVarAssign(info, (VarAssign *) child_block->stmt);} break;
            case AST_RETURNSTMT: {TypecheckReturnStmt(info, (ReturnStmt *) child_block->stmt);} break;
            case AST_IFSTMT:     {TypecheckIfStmt(info, (IfStmt *) child_block->stmt);} break;
            case AST_WHILELOOP:  {TypecheckWhileLoop(info, (WhileLoop *) child_block->stmt);} break;
            case AST_FORLOOP:    {TypecheckForLoop(info, (ForLoop *) child_block->stmt);} break;
            case AST_FUNCCALL:   {TypecheckFuncCall(info, (FuncCall *) child_block->stmt);} break;
            default: {
                printf("invalid statement '%d'\n", child_block->stmt->type);
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
    while (child_file != 0 && child_file->funcdecl != 0) {
        TypecheckFuncDecl(info, child_file->funcdecl);
        child_file = child_file->glue;
    }
}
