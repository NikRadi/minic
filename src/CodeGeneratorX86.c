#include "CodeGeneratorX86.h"
#include "Assembly.h"
#include "Register.h"
#include "ReportError.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void GenerateExpr(struct Expr *expr);
static void GenerateCompoundStmt(struct CompoundStmt *compound_stmt);
static void GenerateDecl(struct AstNode *decl);
static void GenerateStmt(struct AstNode *stmt);

static struct FunctionDef *current_func;
static struct TranslationUnit *current_t_unit;
static FILE *f;

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

static bool IsArray(struct Declarator *decl) {
    return decl->array_dimensions > 0;
}

static bool IsPointer(struct Declarator *decl) {
    return decl->pointer_inderection > 0;
}

static void LoadAddress(struct Expr *expr) {
    // Literals
    if (expr->type == EXPR_VAR) {
        struct List *var_decls = &current_func->var_decls;
        struct Declarator *declarator = FindDeclarator(var_decls, expr->str_value);
        if (!declarator) {
            ReportInternalError("variable address of '%s'", declarator->identifier);
        }

        Lea(RAX, declarator->rbp_offset);
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
            MovImm(RAX, expr->int_value);
        } return;
        case EXPR_STR: {
            struct List *data_fields = &current_t_unit->data_fields;
            for (int i = 0; i < data_fields->count; ++i) {
                struct Expr *data_field = (struct Expr *) List_Get(data_fields, i);
                if (strcmp(expr->str_value, data_field->str_value) == 0) {
                    fprintf(f, "  mov rax, fmt_%d\n", data_field->id);
                    break;
                }
            }
        } return;
        case EXPR_VAR: {
            LoadAddress(expr);

            struct List *var_decls = &current_func->var_decls;
            struct Declarator *declarator = FindDeclarator(var_decls, expr->str_value);
            if (!IsArray(declarator)) {
                struct VarDeclaration *var_decl = FindVarDeclaration(var_decls, expr->str_value);
                if (IsPointer(declarator)) {
                    LoadMem(PRIMTYPE_PTR);
                }
                else {
                    LoadMem(var_decl->type);
                }
            }
        } return;
    }

    // Other operators
    if (expr->type == EXPR_FUNC_CALL) {
        struct List *args = &expr->args;
        for (int i = 0; i < args->count; ++i) {
            struct Expr *arg = (struct Expr *) List_Get(args, i);
            GenerateExpr(arg);
            Push(RAX);
        }

        for (int i = args->count - 1; i >= 0; --i) {
            char **param_reg = param_regs[i];
            Pop(param_reg[PRIMTYPE_PTR]);
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
            Neg(RAX);
        } return;
        case EXPR_DEREF: {
            GenerateExpr(expr->lhs);
            Mov(RAX, "[rax]");
        } return;
        case EXPR_ADDR: {
            LoadAddress(expr->lhs);
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
        Comment("assignment");

        LoadAddress(expr->lhs);
        Push(RAX);
        GenerateExpr(expr->rhs);
        Pop(RDI);
        WriteMemToReg(RDI, rax[expr->lhs->operand_type]);
        if (expr->lhs->operand_type == PRIMTYPE_INVALID) {
            ReportInternalError("CodeGeneratorX86::GenerateExpr - missing operand type");
        }

        return;
    }

    GenerateExpr(expr->rhs);
    Push(RAX);
    GenerateExpr(expr->lhs);
    Pop(RDI);
    switch (expr->type) {
        case EXPR_EQU: { Compare(RAX, RDI, "sete"); } break;
        case EXPR_NEQ: { Compare(RAX, RDI, "setne"); } break;
        case EXPR_LT:  { Compare(RAX, RDI, "setl"); } break;
        case EXPR_GT:  { Compare(RAX, RDI, "setg"); } break;
        case EXPR_LTE: { Compare(RAX, RDI, "setle"); } break;
        case EXPR_GTE: { Compare(RAX, RDI, "setge"); } break;
        case EXPR_ADD: { Add(RAX, RDI); } break;
        case EXPR_SUB: { Sub(RAX, RDI); } break;
        case EXPR_MUL: { Mul(RAX, RDI); } break;
        case EXPR_DIV: { Div(RDI); } break;
        default: { ReportInternalError("CodeGeneratorX86::GenerateExpr - not implemented"); } break;
    }
}

static void GenerateFunctionDef(struct FunctionDef *function) {
    current_func = function;

    // Start at 8 because we call other functions.
    int offset = 8;
    struct List *var_decls = &current_func->var_decls;
    for (int i = 0; i < var_decls->count; ++i) {
        struct VarDeclaration *var_declaration = (struct VarDeclaration *) List_Get(var_decls, i);
        int size_in_bytes = bytes[var_declaration->type];
        if (size_in_bytes == 0) {
            ReportInternalError("SemanticAnalysis::AnalyzeExpr - unexpected sizeof type");
        }

        struct List *declarators = &var_declaration->declarators;
        for (int j = 0; j < declarators->count; ++j) {
            struct Declarator *declarator = (struct Declarator *) List_Get(declarators, j);
            if (IsArray(declarator)) {
                // Consider these two scenarios:
                //  1) int x[3];
                //  2) int x0, x1, x2;
                //
                // While x[0] might seem like it should have the same address as x0,
                // it's actually x[2] that shares the address with x0.
                // The rbp_offset is calculated after considering the full array size.
                int total_vars = 1;
                for (int k = 0; k < declarator->array_dimensions; ++k) {
                    total_vars *= declarator->array_sizes[k];
                }

                offset += size_in_bytes * total_vars;
                declarator->rbp_offset = offset;
            }
            else if (IsPointer(declarator)) {
                offset += bytes[PRIMTYPE_PTR];
                declarator->rbp_offset = offset;
            }
            else {
                offset += size_in_bytes;
                declarator->rbp_offset = offset;
            }

            fprintf(f, "; %s: %d\n", declarator->identifier, declarator->rbp_offset);
        }
    }

    // Reserve 32 bytes for the shadow space.
    // https://stackoverflow.com/questions/30190132/what-is-the-shadow-space-in-x64-assembly/30191127#30191127
    const int shadow_space = 32;
    function->stack_size = Align(offset + shadow_space, 16);
    Label(function->identifier);
    SetupStackFrame(function->stack_size);

    for (int i = 0; i < function->num_params; ++i) {
        struct VarDeclaration *var_decl = (struct VarDeclaration *) List_Get(var_decls, i);
        struct Declarator *decl = (struct Declarator *) List_Get(&var_decl->declarators, 0);

        fprintf(f, "  ; parameter \"%s\"\n", decl->identifier);
        WriteMemOffset(decl->rbp_offset, i, var_decl->type);
    }

    GenerateCompoundStmt(function->body);
    fprintf(f, "return.%s:\n", function->identifier);
    RestoreStackFrame();
    fprintf(f, "\n");
    current_func = NULL;
}

static void GenerateTranslationUnit(struct TranslationUnit *t_unit) {
    current_t_unit = t_unit;
    for (int i = 0; i < t_unit->functions.count; ++i) {
        struct FunctionDef *function = (struct FunctionDef *) List_Get(&t_unit->functions, i);
        GenerateFunctionDef(function);
    }

    current_t_unit = 0;
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
        "forend%d:\n",
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
    SetupAssemblyFile();

    struct List *data_fields = &t_unit->data_fields;
    if (data_fields->count > 0) {
        // TODO: Improve this spaghetti.
        fprintf(f, "section .data\n");
        for (int i = 0; i < data_fields->count; ++i) {
            struct Expr *expr = (struct Expr *) List_Get(data_fields, i);
            expr->id = i;
            char *str = expr->str_value;
            fprintf(f, "  fmt_%d: db \"", i);
            for (int j = 0; str[j] != '\0'; ++j) {
                if (str[j] == '\\') {
                    j += 1;
                    switch (str[j]) {
                        case 'n': { fprintf(f, "\", 10, 0"); } break;
                    }
                }
                else {
                    fprintf(f, "%c", str[j]);
                }
            }

            fprintf(f, "\n");
        }
    }

    fprintf(f,
        "\n"
        "section .text\n"
        "  extern printf\n"
        // Declare the main function as the entry point of the program.
        "  global main\n"
        "\n"
    );

    GenerateTranslationUnit(t_unit);
}
