#ifndef MINIC_AST_NODE_H
#define MINIC_AST_NODE_H
#include "List.h"
#include "Token.h"

enum OperandType {
    OPERAND_INTEGER,
    OPERAND_POINTER,
};

struct AstNode {
    enum AstNodeType {
        AST_DECL,
        AST_DECL_ARRAY,
        AST_FUNCTION_DEFINITION,
        AST_TRANSLATION_UNIT,

        // Statements
        AST_COMPOUND_STMT,
        AST_EXPRESSION_STMT,
        AST_FOR_STMT,
        AST_IF_STMT,
        AST_NULL_STMT,
        AST_RETURN_STMT,
        AST_WHILE_STMT,
    } type;
};

struct Expr {
    struct Expr *lhs;
    struct Expr *rhs;
    struct List args;
    int int_value;
    char str_value[TOKEN_MAX_IDENTIFIER_LENGTH];
    int rbp_offset;
    enum OperandType operand_type;
    enum OperandType base_operand_type;
    enum ExprType {
        // Literals
        EXPR_NUM,
        EXPR_VAR,

        // Others
        EXPR_FUNC_CALL,

        // Unary operators
        EXPR_NEG,       // -lhs
        EXPR_DEREF,     // *lhs
        EXPR_ADDR,      // &lhs

        // Binary operators
        EXPR_EQU,       // lhs == rhs
        EXPR_NEQ,       // lhs != rhs
        EXPR_LT,        // lhs < rhs
        EXPR_GT,        // lhs > rhs
        EXPR_LTE,       // lhs <= rhs
        EXPR_GTE,       // rhs >= rhs
        EXPR_ADD,       // lhs + rhs
        EXPR_SUB,       // lhs - rhs
        EXPR_MUL,       // lhs * rhs
        EXPR_DIV,       // lhs / rhs
        EXPR_ASSIGN,    // lhs = rhs
    } type;
};

struct Decl {
    struct AstNode node;
    char identifier[TOKEN_MAX_IDENTIFIER_LENGTH];
    int rbp_offset;
    int array_size;
};

struct FunctionDefinition {
    struct AstNode node;
    struct CompoundStmt *body;
    struct List var_decls;
    int num_params;
    int stack_size;
    char identifier[TOKEN_MAX_IDENTIFIER_LENGTH];
};

struct TranslationUnit {
    struct AstNode node;
    struct List functions;
};


//
// ===
// == Statements
// ===
//


struct CompoundStmt {
    struct AstNode node;
    struct List stmts;
};

struct ExpressionStmt {
    struct AstNode node;
    struct Expr *expr;
};

struct ForStmt {
    struct AstNode node;
    struct Expr *init_expr;
    struct Expr *cond_expr;
    struct Expr *loop_expr;
    struct AstNode *stmt;
};

struct IfStmt {
    struct AstNode node;
    struct Expr *condition;
    struct AstNode *stmt;
    struct AstNode *else_branch;
};

struct ReturnStmt {
    struct AstNode node;
    struct Expr *expr;
};

struct WhileStmt {
    struct AstNode node;
    struct Expr *condition;
    struct AstNode *stmt;
};


bool AreVariablesEquals(void *a, void *b);

struct Expr *NewFunctionCallExpr(char *identifier, struct List args);
struct Expr *NewOperationAddExpr(struct Expr *lhs, struct Expr *rhs);
struct Expr *NewOperationAddrExpr(struct Expr *lhs);
struct Expr *NewOperationExpr(enum ExprType type, struct Expr *lhs, struct Expr *rhs);
struct Expr *NewOperationSubExpr(struct Expr *lhs, struct Expr *rhs);
struct Expr *NewNumberExpr(int value);
struct Expr *NewVariableExpr(char *identifier);

struct FunctionDefinition *NewFunctionDefinition(char *identifier);
struct TranslationUnit *NewTranslationUnit();

struct CompoundStmt *NewCompoundStmt();
struct ExpressionStmt *NewExpressionStmt(struct Expr *expr);
struct ForStmt *NewForStmt(struct Expr *init_expr, struct Expr *cond_expr, struct Expr *loop_expr, struct AstNode *stmt);
struct IfStmt *NewIfStmt(struct Expr *condition, struct AstNode *stmt, struct AstNode *else_branch);
struct AstNode *NewNullStmt();
struct ReturnStmt *NewReturnStmt(struct Expr *expr);
struct WhileStmt *NewWhileStmt(struct Expr *condition, struct AstNode *stmt);

void PrintE(struct Expr *expr);
void PrintS(struct AstNode *node);

#endif // MINIC_AST_NODE_H
