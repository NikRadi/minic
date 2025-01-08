#include "AstNode.h"
#include "ReportError.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NEW_TYPE(type) ((struct type *) malloc(sizeof(struct type)))

static struct Expr *NewExpr(enum ExprType type) {
    struct Expr *expr = NEW_TYPE(Expr);
    expr->lhs = NULL;
    expr->rhs = NULL;
    expr->int_value = 0;
    expr->rbp_offset = 0;
    expr->type = type;
    return expr;
}

static struct Expr *NewPointerTo(enum ExprType base_type, enum ExprType type, struct Expr *lhs, struct Expr *rhs) {
    struct Expr *expr = NewOperationExpr(type, lhs, rhs);
    expr->base_operand_type = base_type;
    expr->operand_type = OPERAND_POINTER;
    return expr;
}


//
// ===
// == Functions defined in AstNode.h
// ===
//


bool AreVariablesEquals(void *a, void *b) {
    struct Expr *e1 = (struct Expr *) a;
    struct Expr *e2 = (struct Expr *) b;
    return strcmp(e1->str_value, e2->str_value) == 0;
}

struct Expr *NewFunctionCallExpr(char *identifier, struct List args) {
    struct Expr *expr = NewExpr(EXPR_FUNC_CALL);
    strncpy(expr->str_value, identifier, TOKEN_MAX_IDENTIFIER_LENGTH);
    expr->args = args;
    expr->operand_type = OPERAND_INTEGER;
    return expr;
}

struct Expr *NewOperationAddExpr(struct Expr *lhs, struct Expr *rhs) {
    printf("Add operations\n");
    printf("%d %d\n", lhs->operand_type, rhs->operand_type);
    if (lhs->operand_type == OPERAND_INTEGER && rhs->operand_type == OPERAND_INTEGER) {
        return NewOperationExpr(EXPR_ADD, lhs, rhs);
    }

    if (lhs->operand_type == OPERAND_POINTER && rhs->operand_type == OPERAND_INTEGER) {
        struct Expr *num = NewNumberExpr(8);
        rhs = NewPointerTo(rhs->operand_type, EXPR_MUL, num, rhs);
        return NewPointerTo(rhs->operand_type, EXPR_ADD, lhs, rhs);
    }

    if (lhs->operand_type == OPERAND_INTEGER && rhs->operand_type == OPERAND_POINTER) {
        struct Expr *num = NewNumberExpr(8);
        lhs = NewPointerTo(rhs->operand_type, EXPR_MUL, lhs, num);
        return NewPointerTo(rhs->operand_type, EXPR_ADD, lhs, rhs);
    }

    ReportInternalError("binary add operation");
    return NULL;
}

struct Expr *NewOperationAddrExpr(struct Expr *lhs) {
    struct Expr *expr = NewOperationExpr(EXPR_ADDR, lhs, NULL);
    expr->operand_type = OPERAND_POINTER;
    expr->base_operand_type = (lhs->operand_type == OPERAND_POINTER) ? OPERAND_POINTER : OPERAND_INTEGER;
    return expr;
}

struct Expr *NewOperationExpr(enum ExprType type, struct Expr *lhs, struct Expr *rhs) {
    struct Expr *expr = NewExpr(type);
    expr->lhs = lhs;
    expr->rhs = rhs;
    expr->operand_type = OPERAND_INTEGER;
    return expr;
}

struct Expr *NewOperationSubExpr(struct Expr *lhs, struct Expr *rhs) {
    if (lhs->operand_type == OPERAND_INTEGER && rhs->operand_type == OPERAND_INTEGER) {
        return NewOperationExpr(EXPR_SUB, lhs, rhs);
    }

    if (lhs->operand_type == OPERAND_POINTER && rhs->operand_type == OPERAND_INTEGER) {
        struct Expr *num = NewNumberExpr(8);
        rhs = NewPointerTo(rhs->operand_type, EXPR_MUL, num, rhs);
        return NewPointerTo(rhs->operand_type, EXPR_SUB, lhs, rhs);
    }

    if (lhs->operand_type == OPERAND_INTEGER && rhs->operand_type == OPERAND_POINTER) {
        struct Expr *num = NewNumberExpr(8);
        lhs = NewPointerTo(rhs->operand_type, EXPR_MUL, lhs, num);
        return NewPointerTo(rhs->operand_type, EXPR_SUB, lhs, rhs);
    }

    if (lhs->operand_type == OPERAND_POINTER && rhs->operand_type == OPERAND_POINTER) {
        lhs = NewOperationExpr(EXPR_SUB, lhs, rhs);
        struct Expr *num = NewNumberExpr(8);
        struct Expr *expr = NewOperationExpr(EXPR_DIV, lhs, num);
        return expr;
    }

    ReportInternalError("binary sub operation");
    return NULL;
}

struct Expr *NewNumberExpr(int value) {
    struct Expr *expr = NewExpr(EXPR_NUM);
    expr->int_value = value;
    expr->operand_type = OPERAND_INTEGER;
    return expr;
}

struct Expr *NewVariableExpr(char *identifier) {
    struct Expr *expr = NewExpr(EXPR_VAR);
    strncpy(expr->str_value, identifier, TOKEN_MAX_IDENTIFIER_LENGTH);
    expr->operand_type = OPERAND_INTEGER;
    return expr;
}

struct FunctionDefinition *NewFunctionDefinition(char *identifier) {
    struct FunctionDefinition *function = NEW_TYPE(FunctionDefinition);
    function->node.type = AST_FUNCTION_DEFINITION;

    function->num_params = 0;
    function->stack_size = 0;
    strncpy(function->identifier, identifier, TOKEN_MAX_IDENTIFIER_LENGTH);
    function->body = NewCompoundStmt();
    List_Init(&function->var_decls);

    return function;
}

struct TranslationUnit *NewTranslationUnit() {
    struct TranslationUnit *translation_unit = NEW_TYPE(TranslationUnit);
    translation_unit->node.type = AST_TRANSLATION_UNIT;
    List_Init(&translation_unit->functions);
    return translation_unit;
}

struct CompoundStmt *NewCompoundStmt() {
    struct CompoundStmt *compound_stmt = NEW_TYPE(CompoundStmt);
    compound_stmt->node.type = AST_COMPOUND_STMT;
    List_Init(&compound_stmt->stmts);
    return compound_stmt;
}

struct ExpressionStmt *NewExpressionStmt(struct Expr *expr) {
    struct ExpressionStmt *expression_stmt = NEW_TYPE(ExpressionStmt);
    expression_stmt->node.type = AST_EXPRESSION_STMT;
    expression_stmt->expr = expr;
    return expression_stmt;
}

struct ForStmt *NewForStmt(struct Expr *init_expr, struct Expr *cond_expr, struct Expr *loop_expr, struct AstNode *stmt) {
    struct ForStmt *for_stmt = NEW_TYPE(ForStmt);
    for_stmt->node.type = AST_FOR_STMT;
    for_stmt->init_expr = init_expr;
    for_stmt->cond_expr = cond_expr;
    for_stmt->loop_expr = loop_expr;
    for_stmt->stmt = stmt;
    return for_stmt;
}

struct IfStmt *NewIfStmt(struct Expr *condition, struct AstNode *stmt, struct AstNode *else_branch) {
    struct IfStmt *if_stmt = NEW_TYPE(IfStmt);
    if_stmt->node.type = AST_IF_STMT;
    if_stmt->condition = condition;
    if_stmt->stmt = stmt;
    if_stmt->else_branch = else_branch;
    return if_stmt;
}

struct AstNode *NewNullStmt() {
    struct AstNode *null_stmt = NEW_TYPE(AstNode);
    null_stmt->type = AST_NULL_STMT;
    return null_stmt;
}

struct ReturnStmt *NewReturnStmt(struct Expr *expr) {
    struct ReturnStmt *return_stmt = NEW_TYPE(ReturnStmt);
    return_stmt->node.type = AST_RETURN_STMT;
    return_stmt->expr = expr;
    return return_stmt;
}

struct WhileStmt *NewWhileStmt(struct Expr *condition, struct AstNode *stmt) {
    struct WhileStmt *while_stmt = NEW_TYPE(WhileStmt);
    while_stmt->node.type = AST_WHILE_STMT;
    while_stmt->condition = condition;
    while_stmt->stmt = stmt;
    return while_stmt;
}

static int indent = 0;
void PrintE(struct Expr *expr) {
    switch (expr->type) {
        case EXPR_NUM: {
            fprintf(stdout, "%*s<Num int_value=\"%d\" %d %d />\n", indent, "",
                expr->int_value,
                expr->operand_type,
                expr->base_operand_type
            );
        } break;
        case EXPR_VAR: {
            fprintf(stdout, "%*s<Var str_value=\"%s\" %d %d />\n",
                indent, "",
                expr->str_value,
                expr->operand_type,
                expr->base_operand_type
            );
        } break;
        case EXPR_DEREF: {
            fprintf(stdout, "%*s<Deref>\n", indent, "");
            indent += 2;
            PrintE(expr->lhs);
            indent -= 2;
            fprintf(stdout, "%*s</Deref>\n", indent, "");
        } break;
        case EXPR_ADDR: {
            fprintf(stdout, "%*s<Addr %d %d>\n", indent, "",
                expr->operand_type,
                expr->base_operand_type
            );

            indent += 2;
            PrintE(expr->lhs);
            indent -= 2;
            fprintf(stdout, "%*s</Addr>\n", indent, "");
        } break;
        case EXPR_ADD: {
            fprintf(stdout, "%*s<Add>\n", indent, "");
            indent += 2;
            PrintE(expr->lhs);
            PrintE(expr->rhs);
            indent -= 2;
            fprintf(stdout, "%*s</Add>\n", indent, "");
        } break;
        case EXPR_SUB: {
            fprintf(stdout, "%*s<Sub>\n", indent, "");
            indent += 2;
            PrintE(expr->lhs);
            PrintE(expr->rhs);
            indent -= 2;
            fprintf(stdout, "%*s</Sub>\n", indent, "");
        } break;
        case EXPR_ASSIGN: {
            fprintf(stdout, "%*s<Assign>\n", indent, "");
            indent += 2;
            PrintE(expr->lhs);
            PrintE(expr->rhs);
            indent -= 2;
            fprintf(stdout, "%*s</Assign>\n", indent, "");
        } break;
        default: {
            fprintf(stdout, "%*s<UNKNOWN expr %d/>\n", indent, "", expr->type);
        } break;
    }
}

void PrintS(struct AstNode *node) {
    switch (node->type) {
        case AST_DECL: {
            struct Decl *d = (struct Decl *) node;
            fprintf(stdout, "%*s<Decl identifier=\"%s\" rbp_offset=\"%d\"/>\n",
                indent, "",
                d->identifier,
                d->rbp_offset
            );
        } break;
        case AST_DECL_ARRAY: {
            struct Decl *d = (struct Decl *) node;
            fprintf(stdout, "%*s<DeclArray identifier=\"%s\" rbp_offset=\"%d\" array_size=\"%d\"/>\n",
                indent, "",
                d->identifier,
                d->rbp_offset,
                d->array_size
            );
        } break;
        case AST_FUNCTION_DEFINITION: {
            struct FunctionDefinition *f = (struct FunctionDefinition *) node;
            fprintf(stdout, "%*s<FunctionDefinition stack_size=\"%d\">\n", indent, "", f->stack_size);
            indent += 2;
            for (int i = 0; i < f->var_decls.count; ++i) {
                struct AstNode *d = (struct AstNode *) List_Get(&f->var_decls, i);
                PrintS(d);
            }

            PrintS((struct AstNode *) f->body);
            indent -= 2;
            fprintf(stdout, "%*s</FunctionDefinition>\n", indent, "");
        } break;
        case AST_COMPOUND_STMT: {
            struct CompoundStmt *c = (struct CompoundStmt *) node;
            fprintf(stdout, "%*s<CompoundStmt>\n", indent, "");
            indent += 2;
            for (int i = 0; i < c->stmts.count; ++i) {
                struct AstNode *s = (struct AstNode *) List_Get(&c->stmts, i);
                PrintS(s);
            }

            indent -= 2;
            fprintf(stdout, "%*s</CompoundStmt>\n", indent, "");
        } break;
        case AST_EXPRESSION_STMT: {
            struct ExpressionStmt *e = (struct ExpressionStmt *) node;
            fprintf(stdout, "%*s<ExpressionStmt>\n", indent, "");
            indent += 2;
            PrintE(e->expr);
            indent -= 2;
            fprintf(stdout, "%*s</ExpressionStmt>\n", indent, "");
        } break;
        case AST_RETURN_STMT: {
            struct ReturnStmt *r = (struct ReturnStmt *) node;
            fprintf(stdout, "%*s<ReturnStmt>\n", indent, "");
            indent += 2;
            PrintE(r->expr);
            indent -= 2;
            fprintf(stdout, "%*s</ReturnStmt>\n", indent, "");
        } break;
        case AST_TRANSLATION_UNIT: {
            struct TranslationUnit *t = (struct TranslationUnit *) node;
            fprintf(stdout, "%*s<TranslationUnit>\n", indent, "");
            indent += 2;
            for (int i = 0; i < t->functions.count; ++i) {
                struct AstNode *f = (struct AstNode *) List_Get(&t->functions, i);
                PrintS(f);
            }

            indent -= 2;
            fprintf(stdout, "%*s</TranslationUnit>\n", indent, "");
        } break;
        default: {
            fprintf(stdout, "%*s<UNKNOWN statement %d/>\n", indent, "", node->type);
        } break;
    }
}
