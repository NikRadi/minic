#include "CodeGeneratorX86.h"
#include "Assembly.h"
#include "ReportError.h"
#include <stdio.h>

static void GenerateExpression(struct AstNode *expr);
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

static void GenerateAddress(struct AstNode *expr) {
    switch (expr->type) {
        case AST_VARIABLE: {
            void *variable = (void *) expr;
            struct List *variables = &current_function->variables;
            struct Variable *function_variable = (struct Variable *) List_Find(variables, variable, AreVariablesEquals);
            if (!function_variable) {
                ReportInternalError("variable address");
            }

            Lea("rax", function_variable->rbp_offset);
        } break;
        case AST_UNARY_OPERATOR: {
            struct UnaryOp *unary_op = (struct UnaryOp *) expr;
            switch (unary_op->operator_type) {
                case OPERATOR_UNARY_DEREFERENCE: {
                    GenerateExpression(unary_op->expr);
                } break;
                default: {
                    ReportInternalError("address unary op");
                } break;
            }
        } break;
        default: {
            ReportInternalError("address");
        } break;
    }
}

static void GenerateExpression(struct AstNode *expr) {
    switch (expr->type) {
        case AST_LITERAL_NUMBER: {
            struct Literal *literal = (struct Literal *) expr;
            MovImm("rax", literal->int_value);
        } break;
        case AST_BINARY_OPERATOR: {
            struct BinaryOp *binary_op = (struct BinaryOp *) expr;
            if (binary_op->operator_type == OPERATOR_BINARY_ASSIGN) {
                GenerateAddress(binary_op->lhs);
                Push("rax");
                GenerateExpression(binary_op->rhs);
                Pop("rdi");
                Mov("[rdi]", "rax");
            }
            else {
                GenerateExpression(binary_op->rhs);
                Push("rax");
                GenerateExpression(binary_op->lhs);
                Pop("rdi");
                switch (binary_op->operator_type) {
                    case OPERATOR_BINARY_EQUALS:              { Compare("rax", "rdi", "sete");          } break;
                    case OPERATOR_BINARY_NOT_EQUALS:          { Compare("rax", "rdi", "setne");         } break;
                    case OPERATOR_BINARY_LESS_THAN:           { Compare("rax", "rdi", "setl");          } break;
                    case OPERATOR_BINARY_LESS_THAN_EQUALS:    { Compare("rax", "rdi", "setle");         } break;
                    case OPERATOR_BINARY_GREATER_THAN:        { Compare("rax", "rdi", "setg");          } break;
                    case OPERATOR_BINARY_GREATER_THAN_EQUALS: { Compare("rax", "rdi", "setge");         } break;
                    case OPERATOR_BINARY_ADD:                 { Add("rax", "rdi");                      } break;
                    case OPERATOR_BINARY_SUB:                 { Sub("rax", "rdi");                      } break;
                    case OPERATOR_BINARY_MUL:                 { Mul("rax", "rdi");                      } break;
                    case OPERATOR_BINARY_DIV:                 { Div("rdi");                             } break;
                    default:                                  { ReportInternalError("binary op type");  } break;
                }
            }
        } break;
        case AST_UNARY_OPERATOR: {
            struct UnaryOp *unary_op = (struct UnaryOp *) expr;
            switch (unary_op->operator_type) {
                case OPERATOR_UNARY_NEGATE:      { GenerateExpression(unary_op->expr); Neg("rax");          } break;
                case OPERATOR_UNARY_DEREFERENCE: { GenerateExpression(unary_op->expr); Mov("rax", "[rax]"); } break;
                case OPERATOR_UNARY_ADDRESS_OF:  { GenerateAddress(unary_op->expr);                         } break;
                default:                         { ReportInternalError("unary op type");                    } break;
            }
        } break;
        case AST_VARIABLE: { GenerateAddress(expr); Mov("rax", "[rax]"); } break;
        default:           { ReportInternalError("expression");          } break;
    }
}

static void GenerateFunctionDefinition(struct FunctionDefinition *function) {
    current_function = function;

    int offset = 0;
    struct List *function_variables = &function->variables;
    for (int i = function_variables->count - 1; i >= 0; --i) {
        struct Variable* var = (struct Variable *) List_Get(function_variables, i);
        offset += 8;
        var->rbp_offset = offset;
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

static void GenerateForStatement(struct ForStatement *for_statement) {
    if (for_statement->init_expr) GenerateExpression(for_statement->init_expr);
    int label_id = MakeNewLabelId();
    fprintf(f, "forstart%d:\n", label_id);
    if (for_statement->cond_expr) GenerateExpression(for_statement->cond_expr);
    fprintf(f,
        "  cmp rax, 0\n"
        "  je forend%d\n",
        label_id
    );

    GenerateStatement(for_statement->statement);
    if (for_statement->loop_expr) GenerateExpression(for_statement->loop_expr);
    fprintf(f,
        "  jmp forstart%d\n"
        "  forend%d\n",
        label_id,
        label_id
    );
}

static void GenerateIfStatement(struct IfStatement *if_statement) {
    GenerateExpression(if_statement->condition);
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
    if (return_statement->expr) GenerateExpression(return_statement->expr);
    Jmp("return");
}

static void GenerateWhileStatement(struct WhileStatement *while_statement) {
    int label_id = MakeNewLabelId();
    fprintf(f, "whilestart%d:\n", label_id);
    GenerateExpression(while_statement->condition);
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
        case AST_COMPOUND_STATEMENT: { GenerateCompoundStatement((struct CompoundStatement *) statement ); } break;
        case AST_FOR_STATEMENT:      { GenerateForStatement((struct ForStatement *) statement); } break;
        case AST_IF_STATEMENT:       { GenerateIfStatement((struct IfStatement *) statement); } break;
        case AST_NULL_STATEMENT:     { } break;
        case AST_RETURN_STATEMENT:   { GenerateReturnStatement((struct ReturnStatement *) statement); } break;
        case AST_WHILE_STATEMENT:    { GenerateWhileStatement((struct WhileStatement *) statement); } break;
        default:                     { GenerateExpression(statement); } break;
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
