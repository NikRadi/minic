#include "CodeGeneratorX86.h"
#include "Assembly.h"
#include "ReportError.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void GenerateExpr(struct Expr *expr);
static void GenerateCompoundStmt(struct CompoundStmt *compound_stmt);
static void GenerateDecl(struct AstNode *decl);
static void GenerateStmt(struct AstNode *stmt);

static struct FunctionDef *current_func;
static FILE *f;
static char *arg_regs[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };

static int Align(int n, int offset) {
    return (n + offset - 1) / offset * offset;
}

static int MakeNewLabelId() {
    static int num_labels = 0;
    int label_id = num_labels;
    num_labels += 1;
    return label_id;
}

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

static bool IsArray(struct Declarator *declarator) {
    return declarator->array_dimensions > 0;
}

static void GenerateAddress(struct Expr *expr) {
    // Literals
    if (expr->type == EXPR_VAR) {
        struct List *var_decls = &current_func->var_decls;
        struct Declarator *declarator = FindDeclarator(var_decls, expr->str_value);
        if (!declarator) {
            ReportInternalError("variable address of '%s'", declarator->identifier);
        }

        Lea("rax", declarator->rbp_offset);
        return;
    }

    // Unary operators
    if (expr->type == EXPR_DEREF) {
        GenerateExpr(expr->lhs);
        return;
    }
}

static void GenerateExpr(struct Expr *expr) {
    // Literals
    switch (expr->type) {
        case EXPR_NUM: {
            MovImm("rax", expr->int_value);
        } return;
        case EXPR_VAR: {
            GenerateAddress(expr);

            struct List *var_decls = &current_func->var_decls;
            struct Declarator *declarator = FindDeclarator(var_decls, expr->str_value);
            if (!IsArray(declarator)) {
                Mov("rax", "[rax]");
            }
        } return;
    }

    // Other operators
    if (expr->type == EXPR_FUNC_CALL) {
        struct List *args = &expr->args;
        for (int i = 0; i < args->count; ++i) {
            struct Expr *arg = (struct Expr *) List_Get(args, i);
            GenerateExpr(arg);
            Push("rax");
        }

        for (int i = args->count - 1; i >= 0; --i) {
            Pop(arg_regs[i]);
        }

        Call(expr->str_value);
        return;
    }

    // Unary operators
    switch (expr->type) {
        case EXPR_PLUS: {
            GenerateExpr(expr->lhs);
        } return;
        case EXPR_NEG: {
            GenerateExpr(expr->lhs);
            Neg("rax");
        } return;
        case EXPR_DEREF: {
            GenerateExpr(expr->lhs);
            Mov("rax", "[rax]");
        } return;
        case EXPR_ADDR: {
            GenerateAddress(expr->lhs);
        } return;
        case EXPR_SIZEOF: {
            ReportInternalError("CodeGeneratorX86::GenerateExpr - unexpected sizeof");
        } return;
    }

    if (!expr->rhs) {
        ReportInternalError("CodeGeneratorX86::GenerateExpr - missing rhs");
    }

    // Binary operators
    if (expr->type == EXPR_ASSIGN) {
        fprintf(f, "  ; assignment\n");
        GenerateAddress(expr->lhs);
        Push("rax");
        GenerateExpr(expr->rhs);
        Pop("rdi");
        Mov("[rdi]", "rax");
        return;
    }

    GenerateExpr(expr->rhs);
    Push("rax");
    GenerateExpr(expr->lhs);
    Pop("rdi");
    switch (expr->type) {
        case EXPR_EQU: { Compare("rax", "rdi", "sete"); } break;
        case EXPR_NEQ: { Compare("rax", "rdi", "setne"); } break;
        case EXPR_LT:  { Compare("rax", "rdi", "setl"); } break;
        case EXPR_GT:  { Compare("rax", "rdi", "setg"); } break;
        case EXPR_LTE: { Compare("rax", "rdi", "setle"); } break;
        case EXPR_GTE: { Compare("rax", "rdi", "setge"); } break;
        case EXPR_ADD: { Add("rax", "rdi"); } break;
        case EXPR_SUB: { Sub("rax", "rdi"); } break;
        case EXPR_MUL: { Mul("rax", "rdi"); } break;
        case EXPR_DIV: { Div("rdi"); } break;
        default: { ReportInternalError("CodeGeneratorX86::GenerateExpr - not implemented"); } break;
    }
}

static void GenerateFunctionDef(struct FunctionDef *function) {
    current_func = function;

    struct List *var_decls = &current_func->var_decls;
    int offset = 8;
    for (int i = var_decls->count - 1; i >= 0; --i) {
        struct VarDeclaration *var_declaration = (struct VarDeclaration *) List_Get(var_decls, i);
        switch (var_declaration->type) {
            case DECLTYPE_INT: {
                struct List *declarators = &var_declaration->declarators;
                for (int j = 0; j < declarators->count; ++j) {
                    struct Declarator *declarator = (struct Declarator *) List_Get(declarators, j);
                    if (declarator->array_dimensions > 0) {
                        // Consider these two scenarios:
                        //  1) int x[3];
                        //  2) int x0, x1, x2;
                        //
                        // While x[0] might seem like it should have the same address as x0,
                        // it's actually x[2] that shares the address with x0.
                        // The rbp_offset is calculated after considering the full array size.
                        int total_vars = 1;
                        for (int i = 0; i < declarator->array_dimensions; ++i) {
                            total_vars *= declarator->array_sizes[i];
                        }

                        offset += 8 * total_vars;
                        declarator->rbp_offset = offset;
                    }
                    else {
                        declarator->rbp_offset = offset;
                        offset += 8;
                    }
                }
            } break;
            default: {
                printf("%d\n", var_declaration->type);
                ReportInternalError("CodeGeneratorX86::GenerateFunctionDef - unknown var_decl.decl_spec");
            } break;
        }
    }

    function->stack_size = Align(offset, 16);
    Label(function->identifier);
    SetupStackFrame(function->stack_size);

    for (int i = 0; i < function->num_params; ++i) {
        struct VarDeclaration *param = (struct VarDeclaration *) List_Get(var_decls, i);
        struct Declarator *declarator = (struct Declarator *) List_Get(&param->declarators, 0);

        fprintf(f, "; parameter \"%s\"\n", declarator->identifier);
        fprintf(f, "  mov [rbp - %d], %s\n", declarator->rbp_offset, arg_regs[i]);
    }

    GenerateCompoundStmt(function->body);
    fprintf(f, "return.%s:\n", function->identifier);
    RestoreStackFrame();
    current_func = NULL;
}

static void GenerateTranslationUnit(struct TranslationUnit *t_unit) {
    for (int i = 0; i < t_unit->functions.count; ++i) {
        struct FunctionDef *function = (struct FunctionDef *) List_Get(&t_unit->functions, i);
        GenerateFunctionDef(function);
    }
}

static void GenerateCompoundStmt(struct CompoundStmt *compound_stmt) {
    struct List *body = &compound_stmt->body;
    for (int i = 0; i < body->count; ++i) {
        struct AstNode *node = (struct AstNode *) List_Get(body, i);
        if (node->type == AST_VAR_DECLARATION) {
            GenerateDecl(node);
        }
        else {
            GenerateStmt(node);
        }
    }
}

static void GenerateExpressionStmt(struct ExpressionStmt *expression_stmt) {
    GenerateExpr(expression_stmt->expr);
}

static void GenerateForStmt(struct ForStmt *for_stmt) {
    if (for_stmt->init_expr) GenerateExpr(for_stmt->init_expr);
    int label_id = MakeNewLabelId();
    fprintf(f, "forstart%d:\n", label_id);
    if (for_stmt->cond_expr) GenerateExpr(for_stmt->cond_expr);
    fprintf(f,
        "  cmp rax, 0\n"
        "  je forend%d\n",
        label_id
    );

    GenerateStmt(for_stmt->stmt);
    if (for_stmt->loop_expr) GenerateExpr(for_stmt->loop_expr);
    fprintf(f,
        "  jmp forstart%d\n"
        "  forend%d\n",
        label_id,
        label_id
    );
}

static void GenerateIfStmt(struct IfStmt *if_stmt) {
    GenerateExpr(if_stmt->condition);
    int label_id = MakeNewLabelId();
    fprintf(f,
        "  cmp rax, 0\n"
        "  je ifelse%d\n",
        label_id
    );

    GenerateStmt(if_stmt->stmt);
    fprintf(f,
        "  jmp ifend%d\n"
        "ifelse%d:\n",
        label_id,
        label_id
    );

    if (if_stmt->else_branch) GenerateStmt(if_stmt->else_branch);
    fprintf(f, "ifend%d:\n", label_id);
}

static void GenerateReturnStmt(struct ReturnStmt *return_stmt) {
    if (return_stmt->expr) GenerateExpr(return_stmt->expr);
    fprintf(f, "  jmp return.%s\n", current_func->identifier);
}

static void GenerateWhileStmt(struct WhileStmt *while_stmt) {
    int label_id = MakeNewLabelId();
    fprintf(f, "whilestart%d:\n", label_id);
    GenerateExpr(while_stmt->condition);
    fprintf(f,
        "  cmp rax, 0\n"
        "  je whileend%d\n",
        label_id
    );

    GenerateStmt(while_stmt->stmt);
    fprintf(f,
        "  jmp whilestart%d\n"
        "whileend%d:\n",
        label_id,
        label_id
    );
}

static void GenerateVarDeclaration(struct VarDeclaration *var_declaration) {
    struct List *declarators = &var_declaration->declarators;
    for (int i = 0; i < declarators->count; ++i) {
        struct Declarator *declarator = (struct Declarator *) List_Get(declarators, i);
        if (declarator->value) {
            GenerateExpr(declarator->value);
        }
    }
}

static void GenerateDecl(struct AstNode *decl) {
    switch (decl->type) {
        case AST_VAR_DECLARATION:   { GenerateVarDeclaration((struct VarDeclaration *) decl); } break;
        default:                    { ReportInternalError("unknown declaration"); } break;
    }
}

static void GenerateStmt(struct AstNode *stmt) {
    switch (stmt->type) {
        case AST_COMPOUND_STMT:     { GenerateCompoundStmt((struct CompoundStmt *) stmt ); } break;
        case AST_EXPRESSION_STMT:   { GenerateExpressionStmt((struct ExpressionStmt *) stmt); } break;
        case AST_FOR_STMT:          { GenerateForStmt((struct ForStmt *) stmt); } break;
        case AST_IF_STMT:           { GenerateIfStmt((struct IfStmt *) stmt); } break;
        case AST_NULL_STMT:         { } break;
        case AST_RETURN_STMT:       { GenerateReturnStmt((struct ReturnStmt *) stmt); } break;
        case AST_WHILE_STMT:        { GenerateWhileStmt((struct WhileStmt *) stmt); } break;
        default:                    { ReportInternalError("unknown statement"); } break;
    }
}


//
// ===
// == Functions defined in CodeGeneratorX86.h
// ===
//


void CodeGeneratorX86_GenerateCode(FILE *asm_file, struct TranslationUnit *t_unit) {
    current_func = NULL;
    f = asm_file;

    SetOutput(asm_file);
    SetupAssemblyFile("main");

    GenerateTranslationUnit(t_unit);
}
