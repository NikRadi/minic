#include "AstNode.h"
#include "ReportError.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


static bool IsInteger(struct AstNode *node) {
    if (node->type == AST_LITERAL_NUMBER || node->type == AST_VARIABLE) {
        return true;
    }

    if (node->type == AST_BINARY_OPERATOR) {
        struct BinaryOp *binary_op = (struct BinaryOp *) node;
        return binary_op->operand.type == OPERAND_INTEGER;
    }

    if (node->type == AST_UNARY_OPERATOR) {
        struct UnaryOp *unary_op = (struct UnaryOp *) node;
        return unary_op->operand.type == OPERAND_INTEGER;
    }

    return false;
}


//
// ===
// == Functions defined in AstNode.h
// ===
//

bool AreVariablesEquals(void *wanted_element, void *element) {
    struct Variable *wanted_variable = (struct Variable *) wanted_element;
    struct Variable *variable = (struct Variable *) element;
    return strcmp(wanted_variable->identifier, variable->identifier) == 0;
}

enum OperandType BaseOperandTypeOf(struct AstNode *expr) {
    switch (expr->type) {
        case AST_LITERAL_NUMBER:
        case AST_VARIABLE: {
            return OPERAND_INTEGER;
        } break;
        case AST_BINARY_OPERATOR: {
            struct BinaryOp *binary_op = (struct BinaryOp *) expr;
            return binary_op->operand.base_type;
        } break;
        case AST_UNARY_OPERATOR: {
            struct UnaryOp *unary_op = (struct UnaryOp *) expr;
            if (unary_op->operand.base_type == OPERAND_POINTER) {
                return BaseOperandTypeOf(unary_op->expr);
            }

            return unary_op->operand.type;
        } break;
        default: {
            ReportInternalError("base operand");
        } break;
    }
}

struct BinaryOp *NewBinaryAddOp(struct AstNode *lhs, struct AstNode *rhs) {
    struct BinaryOp *binary_op = NewBinaryOp(OPERATOR_BINARY_ADD, lhs, rhs);
    if (IsInteger(lhs) && IsInteger(rhs)) {
        binary_op->operand.type = OPERAND_INTEGER;
    }

    return binary_op;
}

struct BinaryOp *NewBinaryOp(enum OperatorType operator_type, struct AstNode *lhs, struct AstNode *rhs) {
    struct BinaryOp *binary_op = (struct BinaryOp *) malloc(sizeof(struct BinaryOp));
    binary_op->node.type = AST_BINARY_OPERATOR;
    binary_op->operator_type = operator_type;
    binary_op->lhs = lhs;
    binary_op->rhs = rhs;
    return binary_op;
}

struct BinaryOp *NewBinarySubOp(struct AstNode *lhs, struct AstNode *rhs) {
    return NewBinaryOp(OPERATOR_BINARY_SUB, lhs, rhs);
}

struct UnaryOp *NewUnaryAddressOfOp(struct AstNode *expr) {
    struct UnaryOp *unary_op = NewUnaryOp(OPERATOR_UNARY_ADDRESS_OF, expr);
    unary_op->operand.type = OPERAND_POINTER;
    unary_op->operand.base_type = BaseOperandTypeOf(expr);
    return unary_op;
}

struct UnaryOp *NewUnaryOp(enum OperatorType operator_type, struct AstNode *expr) {
    struct UnaryOp *unary_op = (struct UnaryOp *) malloc(sizeof(struct UnaryOp));
    unary_op->node.type = AST_UNARY_OPERATOR;
    unary_op->operator_type = operator_type;
    unary_op->expr = expr;
    return unary_op;
}
