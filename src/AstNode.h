#ifndef MINIC_AST_NODE_H
#define MINIC_AST_NODE_H
#include "List.h"
#include "Token.h"

#define MAX_ARRAY_DIMENSIONS 4


enum PrimitiveType {
    PRIMTYPE_INVALID,
    PRIMTYPE_CHAR,
    PRIMTYPE_INT,
    PRIMTYPE_PTR,
    PRIMTYPE_COUNT,
};

struct AstNode {
    enum AstNodeType {
        AST_INVALID,
        AST_DECLARATOR,
        AST_VAR_DECLARATION,
        AST_FUNCTION_DEF,
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
    struct List args; // Used for EXPR_FUNC_CALL.
    enum PrimitiveType operand_type;
    enum PrimitiveType base_operand_type;
    int int_value;
    int id; // Temporarily used for string ids
    char str_value[TOKEN_MAX_IDENTIFIER_LENGTH];
    int rbp_offset;
    enum ExprType {
        EXPR_INVALID,

        // Literals
        EXPR_NUM,
        EXPR_STR,
        EXPR_VAR,

        // Others
        EXPR_FUNC_CALL,

        // Unary operators
        EXPR_PLUS,      // +lhs
        EXPR_NEG,       // -lhs
        EXPR_DEREF,     // *lhs
        EXPR_ADDR,      // &lhs
        EXPR_SIZEOF,    // sizeof(lhs)

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

struct Declarator {
    struct AstNode node;
    struct Expr *value;
    int array_dimensions;
    int array_sizes[MAX_ARRAY_DIMENSIONS];
    int pointer_inderection;
    int rbp_offset;
    char identifier[TOKEN_MAX_IDENTIFIER_LENGTH];
};

struct VarDeclaration {
    struct AstNode node;
    struct List declarators;
    enum PrimitiveType type;
};

struct FunctionDef {
    struct AstNode node;
    struct CompoundStmt *body;
    struct List params;
    struct List var_decls;
    enum PrimitiveType return_type;
    int num_params;
    int stack_size;
    char identifier[TOKEN_MAX_IDENTIFIER_LENGTH];
};

struct TranslationUnit {
    struct AstNode node;
    struct List functions;
    struct List data_fields;
};


//
// ===
// == Statements
// ===
//


struct CompoundStmt {
    struct AstNode node;
    struct List body;
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


struct Expr *NewFunctionCallExpr(char *identifier, struct List args);
struct Expr *NewOperationExpr(enum ExprType type, struct Expr *lhs, struct Expr *rhs);
struct Expr *NewNumberExpr(int value);
struct Expr *NewStringExpr(char *value);
struct Expr *NewVariableExpr(char *identifier);

struct Declarator *NewDeclarator();
struct VarDeclaration *NewVarDeclaration();
struct FunctionDef *NewFunctionDef(char *identifier, enum PrimitiveType return_type);
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
