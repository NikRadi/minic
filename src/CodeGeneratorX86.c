#include "CodeGeneratorX86.h"
#include "Assembly.h"
#include "ReportError.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void GenerateExpr(struct Expr *expr);
static void GenerateStatement(struct AstNode *statement);

static struct FunctionDefinition *current_function;
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

static void GenerateAddress(struct Expr *expr) {
    // Literals
    if (expr->type == EXPR_VAR) {
        struct List *var_declarations = &current_function->var_declarations;
        struct Declaration *var_declaration = NULL;
        for (int i = 0; i < var_declarations->count; ++i) {
            struct Declaration *v = (struct Declaration *) List_Get(var_declarations, i);
            if (strcmp(v->identifier, expr->str_value) == 0) {
                var_declaration = v;
                break;
            }
        }

        if (!var_declaration) {
            ReportInternalError("variable address of '%s'", expr->str_value);
        }

        Lea("rax", var_declaration->rbp_offset);
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
            Mov("rax", "[rax]");
        } return;
    }

    // Unary operators
    switch (expr->type) {
        case EXPR_ADDR: {
            GenerateAddress(expr->lhs);
        } return;
        case EXPR_DEREF: {
            GenerateExpr(expr->lhs);
            Mov("rax", "[rax]");
        } return;
        case EXPR_NEG: {
            GenerateExpr(expr->lhs);
            Neg("rax");
        } return;
    }

    // Binary operators
    if (expr->type == EXPR_ASSIGN) {
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
        default: { ReportInternalError("not implemented"); } break;
    }
}

static void GenerateFunctionDefinition(struct FunctionDefinition *function) {
    current_function = function;

    int offset = 0;
    struct List *var_declarations = &current_function->var_declarations;
    for (int i = var_declarations->count - 1; i >= 0; --i) {
        struct Declaration *var_declaration = (struct Declaration *) List_Get(var_declarations, i);
        offset += 8;
        var_declaration->rbp_offset = offset;
    }

    function->stack_size = Align(offset, 16);
    Label("main");
    SetupStackFrame(function->stack_size);

    struct List *statements = &function->statements;
    for (int i = 0; i < statements->count; ++i) {
        struct AstNode *statement = (struct AstNode *) List_Get(statements, i);
        GenerateStatement(statement);
    }

    Label("return");
    RestoreStackFrame();
    current_function = NULL;
}


//
// ===
// == Generate statements
// ===
//


static void GenerateCompoundStatement(struct CompoundStatement *compound_statement) {
    struct List *statements = &compound_statement->statements;
    for (int i = 0; i < statements->count; ++i) {
        struct AstNode *statement = (struct AstNode *) List_Get(statements, i);
        GenerateStatement(statement);
    }
}

static void GenerateExpressionStatement(struct ExpressionStatement *expression_statement) {
    GenerateExpr(expression_statement->expr);
}

static void GenerateForStatement(struct ForStatement *for_statement) {
    if (for_statement->init_expr) GenerateExpr(for_statement->init_expr);
    int label_id = MakeNewLabelId();
    fprintf(f, "forstart%d:\n", label_id);
    if (for_statement->cond_expr) GenerateExpr(for_statement->cond_expr);
    fprintf(f,
        "  cmp rax, 0\n"
        "  je forend%d\n",
        label_id
    );

    GenerateStatement(for_statement->statement);
    if (for_statement->loop_expr) GenerateExpr(for_statement->loop_expr);
    fprintf(f,
        "  jmp forstart%d\n"
        "  forend%d\n",
        label_id,
        label_id
    );
}

static void GenerateIfStatement(struct IfStatement *if_statement) {
    GenerateExpr(if_statement->condition);
    int label_id = MakeNewLabelId();
    fprintf(f,
        "  cmp rax, 0\n"
        "  je ifelse%d\n",
        label_id
    );

    GenerateStatement(if_statement->statement);
    fprintf(f,
        "  jmp ifend%d\n"
        "ifelse%d:\n",
        label_id,
        label_id
    );

    if (if_statement->else_branch) GenerateStatement(if_statement->else_branch);
    fprintf(f, "ifend%d:\n", label_id);
}

static void GenerateReturnStatement(struct ReturnStatement *return_statement) {
    if (return_statement->expr) GenerateExpr(return_statement->expr);
    Jmp("return");
}

static void GenerateWhileStatement(struct WhileStatement *while_statement) {
    int label_id = MakeNewLabelId();
    fprintf(f, "whilestart%d:\n", label_id);
    GenerateExpr(while_statement->condition);
    fprintf(f,
        "  cmp rax, 0\n"
        "  je whileend%d\n",
        label_id
    );

    GenerateStatement(while_statement->statement);
    fprintf(f,
        "  jmp whilestart%d\n"
        "whileend%d:\n",
        label_id,
        label_id
    );
}

static void GenerateStatement(struct AstNode *statement) {
    switch (statement->type) {
        case AST_COMPOUND_STATEMENT:    { GenerateCompoundStatement((struct CompoundStatement *) statement ); } break;
        case AST_EXPRESSION_STATEMENT:  { GenerateExpressionStatement((struct ExpressionStatement *) statement); } break;
        case AST_FOR_STATEMENT:         { GenerateForStatement((struct ForStatement *) statement); } break;
        case AST_IF_STATEMENT:          { GenerateIfStatement((struct IfStatement *) statement); } break;
        case AST_NULL_STATEMENT:        { } break;
        case AST_RETURN_STATEMENT:      { GenerateReturnStatement((struct ReturnStatement *) statement); } break;
        case AST_WHILE_STATEMENT:       { GenerateWhileStatement((struct WhileStatement *) statement); } break;
        default:                        { ReportInternalError("unknown statement"); } break;
    }
}


//
// ===
// == Functions defined in CodeGeneratorX86.h
// ===
//


void CodeGeneratorX86_GenerateCode(FILE *asm_file, struct FunctionDefinition *function) {
    current_function = NULL;
    f = asm_file;

    SetOutput(asm_file);
    SetupAssemblyFile("main");

    GenerateFunctionDefinition(function);
}
