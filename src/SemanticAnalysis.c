#include "SemanticAnalysis.h"
#include "Sizes.h"
#include "ReportError.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static void AnalyzeStmt(struct AstNode *stmt);
static void AnalyzeCompoundStmt(struct CompoundStmt *compound_stmt);


static struct FunctionDef *current_func;
static struct TranslationUnit *current_t_unit;


static struct Declarator *FindDeclarator(struct List *var_declarations, char *identifier) {
    for (int i = 0; i < var_declarations->count; ++i) {
        struct VarDeclaration *var_declaration = (struct VarDeclaration *) List_Get(var_declarations, i);
        struct List *declarators = &var_declaration->declarators;
        for (int j = 0; j < declarators->count; ++j) {
            struct Declarator *declarator = (struct Declarator *) List_Get(declarators, j);
            if (strcmp(declarator->identifier, identifier) == 0) {
                return declarator;
            }
        }
    }

    return NULL;
}

static struct VarDeclaration *FindVarDeclaration(struct List *var_declarations, char *identifier) {
    for (int i = 0; i < var_declarations->count; ++i) {
        struct VarDeclaration *var_declaration = (struct VarDeclaration *) List_Get(var_declarations, i);
        struct List *declarators = &var_declaration->declarators;
        for (int j = 0; j < declarators->count; ++j) {
            struct Declarator *declarator = (struct Declarator *) List_Get(declarators, j);
            if (strcmp(declarator->identifier, identifier) == 0) {
                return var_declaration;
            }
        }
    }

    return NULL;
}

static void AnalyzeExpr(struct Expr *expr) {
    switch (expr->type) {
        case EXPR_NUM: {
            expr->operand_type = PRIMTYPE_INT;
        } break;
        case EXPR_STR: {
            bool string_exists = false;
            struct List *data_fields = &current_t_unit->data_fields;
            for (int i = 0; i < data_fields->count; ++i) {
                struct Expr *data_field = (struct Expr *) List_Get(data_fields, i);
                if (strcmp(expr->str_value, data_field->str_value) == 0) {
                    string_exists = true;
                    break;
                }
            }

            if (!string_exists) {
                List_Add(data_fields, expr);
            }
        } break;
        case EXPR_VAR: {
            struct Declarator *decl = FindDeclarator(&current_func->var_decls, expr->str_value);
            if (!decl) {
                printf("err: couldnt find decl\n");
            }

            if (decl->array_dimensions > 0 || decl->pointer_inderection > 0) {
                expr->operand_type = PRIMTYPE_PTR;
                if (decl->array_dimensions > 1 || decl->pointer_inderection > 1) {
                    expr->base_operand_type = PRIMTYPE_PTR;
                }
                else {
                    expr->base_operand_type = PRIMTYPE_INT;
                }
            }
            else {
                struct VarDeclaration *var_decl = FindVarDeclaration(&current_func->var_decls, expr->str_value);
                expr->operand_type = var_decl->type;
            }
        } break;
        case EXPR_FUNC_CALL: {
            struct List *args = &expr->args;
            for (int i = 0; i < args->count; ++i) {
                struct Expr *arg = (struct Expr *) List_Get(args, i);
                AnalyzeExpr(arg);
            }
        } break;
        case EXPR_NEG: {
            AnalyzeExpr(expr->lhs);
            expr->operand_type = expr->lhs->operand_type;
        } break;
        case EXPR_DEREF: {
            AnalyzeExpr(expr->lhs);
            expr->operand_type = expr->lhs->operand_type;
        } break;
        case EXPR_ADDR: {
            AnalyzeExpr(expr->lhs);
            expr->operand_type = PRIMTYPE_PTR;
            expr->base_operand_type = expr->lhs->operand_type;
        } break;
        case EXPR_SIZEOF: {
            AnalyzeExpr(expr->lhs);
            expr->type = EXPR_NUM;
            expr->int_value = bytes[expr->lhs->operand_type];
            if (expr->int_value == 0) {
                ReportInternalError("SemanticAnalysis::AnalyzeExpr - unexpected sizeof type");
            }
        } break;
        case EXPR_ADD:
        case EXPR_SUB: {
            AnalyzeExpr(expr->lhs);
            AnalyzeExpr(expr->rhs);
            if (expr->lhs->operand_type == PRIMTYPE_PTR && expr->rhs->operand_type == PRIMTYPE_INT) {
                struct Expr *num = NewNumberExpr(8);
                struct Expr *new_rhs = NewOperationExpr(EXPR_MUL, num, expr->rhs);
                expr->rhs = new_rhs;
                expr->operand_type = PRIMTYPE_PTR;
            }
            else if (expr->lhs->operand_type == PRIMTYPE_INT && expr->rhs->operand_type == PRIMTYPE_PTR) {
                struct Expr *num = NewNumberExpr(8);
                struct Expr *new_lhs = NewOperationExpr(EXPR_MUL, num, expr->lhs);
                expr->lhs = new_lhs;
                expr->operand_type = PRIMTYPE_PTR;
            }
            else if (expr->type == EXPR_SUB && expr->lhs->operand_type == PRIMTYPE_PTR && expr->rhs->operand_type == PRIMTYPE_PTR) {
                expr->type = EXPR_DIV;
                expr->lhs = NewOperationExpr(EXPR_SUB, expr->lhs, expr->rhs);
                expr->rhs = NewNumberExpr(8);
            }
            else {
                expr->operand_type = PRIMTYPE_INT;
            }
        } break;
        case EXPR_ASSIGN: {
            AnalyzeExpr(expr->lhs);
            AnalyzeExpr(expr->rhs);
            expr->operand_type = expr->lhs->operand_type;
        } break;
    }
}

static void AnalyzeVarDecl(struct VarDeclaration *var_declaration) {
    List_Add(&current_func->var_decls, var_declaration);

    struct List *declarators = &var_declaration->declarators;
    for (int i = 0; i < declarators->count; ++i) {
        struct Declarator *declarator = (struct Declarator *) List_Get(declarators, i);
        if (declarator->value) {
            AnalyzeExpr(declarator->value);
        }
    }
}

static void AnalyzeExpressionStmt(struct ExpressionStmt *expr_stmt) {
    AnalyzeExpr(expr_stmt->expr);
}

static void AnalyzeForStmt(struct ForStmt *for_stmt) {
    AnalyzeExpr(for_stmt->init_expr);
    AnalyzeExpr(for_stmt->cond_expr);
    AnalyzeExpr(for_stmt->loop_expr);
    AnalyzeStmt(for_stmt->stmt);
}

static void AnalyzeIfStmt(struct IfStmt *if_stmt) {
    AnalyzeExpr(if_stmt->condition);
    AnalyzeStmt(if_stmt->stmt);
}

static void AnalyzeReturnStmt(struct ReturnStmt *return_stmt) {
    if (return_stmt->expr) {
        AnalyzeExpr(return_stmt->expr);
    }
}

static void AnalyzeWhileStmt(struct WhileStmt *while_stmt) {
    AnalyzeExpr(while_stmt->condition);
    AnalyzeStmt(while_stmt->stmt);
}

static void AnalyzeDecl(struct AstNode *decl) {
    switch (decl->type) {
        case AST_VAR_DECLARATION:   { AnalyzeVarDecl((struct VarDeclaration *) decl); } break;
    }
}

static void AnalyzeStmt(struct AstNode *stmt) {
    switch (stmt->type) {
        case AST_EXPRESSION_STMT:   { AnalyzeExpressionStmt((struct ExpressionStmt *) stmt); } break;
        case AST_COMPOUND_STMT:     { AnalyzeCompoundStmt((struct CompoundStmt *) stmt); } break;
        case AST_FOR_STMT:          { AnalyzeForStmt((struct ForStmt *) stmt); } break;
        case AST_IF_STMT:           { AnalyzeIfStmt((struct IfStmt *) stmt); } break;
        case AST_RETURN_STMT:       { AnalyzeReturnStmt((struct ReturnStmt *) stmt); } break;
        case AST_WHILE_STMT:        { AnalyzeWhileStmt((struct WhileStmt *) stmt); } break;
    }
}

static void AnalyzeCompoundStmt(struct CompoundStmt *compound_stmt) {
    struct List *body = &compound_stmt->body;
    for (int i = 0; i < body->count; ++i) {
        struct AstNode *node = (struct AstNode *) List_Get(body, i);
        if (node->type == AST_VAR_DECLARATION) {
            AnalyzeDecl(node);
        }
        else {
            AnalyzeStmt(node);
        }
    }
}

static void AnalyzeFunctionDef(struct FunctionDef *func) {
    current_func = func;
    AnalyzeCompoundStmt(func->body);
    current_func = 0;
}

static void AnalyzeTranslationUnit(struct TranslationUnit *t_unit) {
    current_t_unit = t_unit;
    struct List *functions = &t_unit->functions;
    for (int i = 0; i < functions->count; ++i) {
        struct FunctionDef *func = (struct FunctionDef *) List_Get(functions, i);
        AnalyzeFunctionDef(func);
    }

    current_t_unit = 0;
}

//
// ===
// == Functions defined in SemanticAnalysis.h
// ===
//

void SemanticAnalysis_Analyze(struct TranslationUnit *t_unit) {
    AnalyzeTranslationUnit(t_unit);
}
