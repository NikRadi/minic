#ifndef MINIC_AST_NODE_H
#define MINIC_AST_NODE_H
#include "List.h"
#include "Token.h"

struct Operand;

enum AstNodeType {
    AST_BINARY_OPERATOR,
    AST_COMPOUND_STATEMENT,
    AST_FOR_LOOP,
    AST_FUNCTION_DEFINITION,
    AST_IF_STATEMENT,
    AST_LITERAL_NUMBER,
    AST_RETURN_STATEMENT,
    AST_UNARY_OPERATOR,
    AST_VARIABLE,
    AST_WHILE_LOOP,
};

enum OperatorType {
    OPERATOR_INVALID,
    OPERATOR_BINARY_ASSIGN,
    OPERATOR_BINARY_EQUALS,
    OPERATOR_BINARY_NOT_EQUALS,
    OPERATOR_BINARY_LESS_THAN,
    OPERATOR_BINARY_LESS_THAN_EQUALS,
    OPERATOR_BINARY_GREATER_THAN,
    OPERATOR_BINARY_GREATER_THAN_EQUALS,
    OPERATOR_BINARY_SUB,
    OPERATOR_BINARY_ADD,
    OPERATOR_BINARY_MUL,
    OPERATOR_BINARY_DIV,
    OPERATOR_BINARY_MODULO,
//    OPERATOR_UNARY_PLUS, // Leave it out because it doesn't do anything
    OPERATOR_UNARY_NEGATE,
    OPERATOR_UNARY_DEREFERENCE,
    OPERATOR_UNARY_ADDRESS_OF,
};

enum OperandType {
    OPERAND_INVALID,
    OPERAND_INTEGER,
    OPERAND_POINTER,
};

struct AstNode {
    enum AstNodeType type;
};

struct Operand {
    enum OperandType type;
    enum OperandType base_type; // Used if type is OPERAND_POINTER
};

struct BinaryOp {
    struct AstNode node;
    struct Operand operand;
    enum OperatorType operator_type;
    struct AstNode *lhs;
    struct AstNode *rhs;
};

struct CompoundStatement {
    struct AstNode node;
    struct List statements;
};

struct FunctionDefinition {
    struct AstNode node;
    struct List statements;
    struct List variables;
    int stack_size;
};

struct ForLoop {
    struct AstNode node;
    struct AstNode *init_statement;
    struct AstNode *condition;
    struct AstNode *update_expr;
    struct AstNode *statement;
};

struct IfStatement {
    struct AstNode node;
    struct AstNode *condition;
    struct AstNode *statement;
    struct AstNode *else_block;
};

struct Literal {
    struct AstNode node;
    struct Operand operand;
    union {
        int int_value;
    };
};

struct ReturnStatement {
    struct AstNode node;
    struct AstNode *expr;
};

struct UnaryOp {
    struct AstNode node;
    struct AstNode *expr;
    struct Operand operand;
    enum OperatorType operator_type;
};

struct Variable {
    struct AstNode node;
    struct Operand operand;
    char identifier[TOKEN_MAX_IDENTIFIER_LENGTH];
    int rbp_offset;
};

struct WhileLoop {
    struct AstNode node;
    struct AstNode *condition;
    struct AstNode *statement;
};

bool AreVariablesEquals(void *wanted_element, void *element);

enum OperandType BaseOperandTypeOf(struct AstNode *expr);

struct BinaryOp *NewBinaryAddOp(struct AstNode *lhs, struct AstNode *rhs);

struct BinaryOp *NewBinaryOp(enum OperatorType operator_type, struct AstNode *lhs, struct AstNode *rhs);

struct BinaryOp *NewBinarySubOp(struct AstNode *lhs, struct AstNode *rhs);

struct UnaryOp *NewUnaryAddressOfOp(struct AstNode *expr);

struct UnaryOp *NewUnaryOp(enum OperatorType operator_type, struct AstNode *expr);

void Print(struct AstNode *node);

#endif // MINIC_AST_NODE_H
