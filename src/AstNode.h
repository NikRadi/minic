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
        AST_DECLARATION,
        AST_FUNCTION_DEFINITION,

        // Statements
        AST_COMPOUND_STATEMENT,
        AST_EXPRESSION_STATEMENT,
        AST_FOR_STATEMENT,
        AST_IF_STATEMENT,
        AST_NULL_STATEMENT,
        AST_RETURN_STATEMENT,
        AST_WHILE_STATEMENT,
    } type;
};

struct Expr {
    struct Expr *lhs;
    struct Expr *rhs;
    int int_value;
    char str_value[TOKEN_MAX_IDENTIFIER_LENGTH];
    int rbp_offset;
    enum OperandType operand_type;
    enum OperandType base_operand_type;
    enum ExprType {
        // Literals
        EXPR_NUM,
        EXPR_VAR,

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

        // Others
        EXPR_FUNC_CALL,
    } type;
};

struct Declaration {
    struct AstNode node;
    char identifier[TOKEN_MAX_IDENTIFIER_LENGTH];
    int rbp_offset;
};

struct FunctionDefinition {
    struct AstNode node;
    struct List statements;
    struct List var_declarations;
    int stack_size;
};


//
// ===
// == Statements
// ===
//


struct CompoundStatement {
    struct AstNode node;
    struct List statements;
};

struct ExpressionStatement {
    struct AstNode node;
    struct Expr *expr;
};

struct ForStatement {
    struct AstNode node;
    struct Expr *init_expr;
    struct Expr *cond_expr;
    struct Expr *loop_expr;
    struct AstNode *statement;
};

struct IfStatement {
    struct AstNode node;
    struct Expr *condition;
    struct AstNode *statement;
    struct AstNode *else_branch;
};

struct ReturnStatement {
    struct AstNode node;
    struct Expr *expr;
};

struct WhileStatement {
    struct AstNode node;
    struct Expr *condition;
    struct AstNode *statement;
};


bool AreVariablesEquals(void *a, void *b);

struct Expr *NewFunctionCallExpr(char *identifier);
struct Expr *NewOperationAddExpr(struct Expr *lhs, struct Expr *rhs);
struct Expr *NewOperationAddrExpr(struct Expr *lhs);
struct Expr *NewOperationExpr(enum ExprType type, struct Expr *lhs, struct Expr *rhs);
struct Expr *NewOperationSubExpr(struct Expr *lhs, struct Expr *rhs);
struct Expr *NewNumberExpr(int value);
struct Expr *NewVariableExpr(char *identifier);

struct CompoundStatement *NewCompoundStatement(struct List statements);
struct ExpressionStatement *NewExpressionStatement(struct Expr *expr);
struct ForStatement *NewForStatement(struct Expr *init_expr, struct Expr *cond_expr, struct Expr *loop_expr, struct AstNode *statement);
struct IfStatement *NewIfStatement(struct Expr *condition, struct AstNode *statement, struct AstNode *else_branch);
struct AstNode *NewNullStatement();
struct ReturnStatement *NewReturnStatement(struct Expr *expr);
struct WhileStatement *NewWhileStatement(struct Expr *condition, struct AstNode *statement);

void PrintE(struct Expr *expr);
void PrintS(struct AstNode *node);

#endif // MINIC_AST_NODE_H
