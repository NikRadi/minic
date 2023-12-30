#include "AstNode.h"
#include "ReportError.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static enum OperandType OperandTypeOf(struct AstNode *node) {
    enum OperandType node_operand_type = OPERAND_INVALID;
    switch (node->type) {
        case AST_LITERAL_NUMBER:    { node_operand_type = ((struct Literal *) node)->operand.type;  } break;
        case AST_VARIABLE:          { node_operand_type = ((struct Variable *) node)->operand.type; } break;
        case AST_BINARY_OPERATOR:   { node_operand_type = ((struct BinaryOp *) node)->operand.type; } break;
        case AST_UNARY_OPERATOR:    { node_operand_type = ((struct UnaryOp *) node)->operand.type;  } break;
    }

    return node_operand_type;
}

static bool IsOperandType(struct AstNode *node, enum OperandType type) {
    return OperandTypeOf(node) == type;
}

static void PrintOperatorType(enum OperatorType type) {
    switch (type) {
        case OPERATOR_BINARY_ASSIGN:                { printf("="); } break;
        case OPERATOR_BINARY_EQUALS:                { printf("=="); } break;
        case OPERATOR_BINARY_NOT_EQUALS:            { printf("!="); } break;
        case OPERATOR_BINARY_LESS_THAN:             { printf("<"); } break;
        case OPERATOR_BINARY_LESS_THAN_EQUALS:      { printf("<="); } break;
        case OPERATOR_BINARY_GREATER_THAN:          { printf(">"); } break;
        case OPERATOR_BINARY_GREATER_THAN_EQUALS:   { printf(">="); } break;
        case OPERATOR_BINARY_SUB:                   { printf("-"); } break;
        case OPERATOR_BINARY_ADD:                   { printf("+"); } break;
        case OPERATOR_BINARY_MUL:                   { printf("*"); } break;
        case OPERATOR_BINARY_DIV:                   { printf("/"); } break;
        case OPERATOR_BINARY_MODULO:                { printf(":)"); } break;
        case OPERATOR_UNARY_NEGATE:                 { printf("-"); } break;
        case OPERATOR_UNARY_DEREFERENCE:            { printf("*"); } break;
        case OPERATOR_UNARY_ADDRESS_OF:             { printf("&"); } break;
        default: { printf("?"); } break;
    }
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
    if (IsOperandType(lhs, OPERAND_INTEGER) && IsOperandType(rhs, OPERAND_INTEGER)) {
        return NewBinaryOp(OPERATOR_BINARY_ADD, lhs, rhs);
    }

    if (IsOperandType(lhs, OPERAND_POINTER) && IsOperandType(rhs, OPERAND_INTEGER)) {
        struct Literal *literal = (struct Literal *) malloc(sizeof(struct Literal));
        literal->node.type = AST_LITERAL_NUMBER;
        literal->int_value = 8;
        literal->operand.type = OPERAND_INTEGER;

        rhs = (struct AstNode *) NewBinaryOp(OPERATOR_BINARY_MUL, (struct AstNode *) literal, rhs);
        return NewBinaryOp(OPERATOR_BINARY_ADD, lhs, rhs);
    }

    if (IsOperandType(lhs, OPERAND_INTEGER) && IsOperandType(rhs, OPERAND_POINTER)) {
        struct Literal *literal = (struct Literal *) malloc(sizeof(struct Literal));
        literal->node.type = AST_LITERAL_NUMBER;
        literal->int_value = 8;
        literal->operand.type = OPERAND_INTEGER;

        lhs = (struct AstNode *) NewBinaryOp(OPERATOR_BINARY_MUL, lhs, (struct AstNode *) literal);
        return NewBinaryOp(OPERATOR_BINARY_ADD, lhs, rhs);
    }

    ReportInternalError("binary add operation");
    return NULL;
}

struct BinaryOp *NewBinaryOp(enum OperatorType operator_type, struct AstNode *lhs, struct AstNode *rhs) {
    struct BinaryOp *binary_op = (struct BinaryOp *) malloc(sizeof(struct BinaryOp));
    binary_op->node.type = AST_BINARY_OPERATOR;
    binary_op->operator_type = operator_type;
    binary_op->lhs = lhs;
    binary_op->rhs = rhs;
    binary_op->operand.type = OperandTypeOf(lhs);
    return binary_op;
}

struct BinaryOp *NewBinarySubOp(struct AstNode *lhs, struct AstNode *rhs) {
    if (IsOperandType(lhs, OPERAND_INTEGER) && IsOperandType(rhs, OPERAND_INTEGER)) {
        return NewBinaryOp(OPERATOR_BINARY_SUB, lhs, rhs);
    }

    if (IsOperandType(lhs, OPERAND_POINTER) && IsOperandType(rhs, OPERAND_INTEGER)) {
        struct Literal *literal = (struct Literal *) malloc(sizeof(struct Literal));
        literal->node.type = AST_LITERAL_NUMBER;
        literal->int_value = 8;
        literal->operand.type = OPERAND_INTEGER;

        rhs = (struct AstNode *) NewBinaryOp(OPERATOR_BINARY_MUL, (struct AstNode *) literal, rhs);
        return NewBinaryOp(OPERATOR_BINARY_SUB, lhs, rhs);
    }

    if (IsOperandType(lhs, OPERAND_INTEGER) && IsOperandType(rhs, OPERAND_POINTER)) {
        struct Literal *literal = (struct Literal *) malloc(sizeof(struct Literal));
        literal->node.type = AST_LITERAL_NUMBER;
        literal->int_value = 8;
        literal->operand.type = OPERAND_INTEGER;

        lhs = (struct AstNode *) NewBinaryOp(OPERATOR_BINARY_MUL, lhs, (struct AstNode *) literal);
        return NewBinaryOp(OPERATOR_BINARY_SUB, lhs, rhs);
    }

    if (IsOperandType(lhs, OPERAND_POINTER) && IsOperandType(rhs, OPERAND_POINTER)) {
        lhs = (struct AstNode *) NewBinaryOp(OPERATOR_BINARY_SUB, lhs, rhs);
        struct Literal *literal = (struct Literal *) malloc(sizeof(struct Literal));
        literal->node.type = AST_LITERAL_NUMBER;
        literal->int_value = 8;
        literal->operand.type = OPERAND_INTEGER;

        struct BinaryOp *binary_op = NewBinaryOp(OPERATOR_BINARY_DIV, lhs, (struct AstNode *) literal);
        binary_op->operand.type = OPERAND_INTEGER;
        return binary_op;
    }

    ReportInternalError("binary sub operation");
    return NULL;
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
    unary_op->operand.type = OPERAND_INTEGER;
    unary_op->expr = expr;
    return unary_op;
}

void Print(struct AstNode *node) {
    static int indent = 0;
    switch (node->type) {
        case AST_BINARY_OPERATOR: {
            struct BinaryOp *b = (struct BinaryOp *) node;
            printf("%*s<BinaryOp operator_type=\"", indent, "");
            PrintOperatorType(b->operator_type);
            printf("\" operand=\"{ %d, %d }\" />\n", b->operand.type, b->operand.base_type);

            indent += 2;
            Print(b->lhs);
            Print(b->rhs);
            indent -= 2;
            printf("%*s</BinaryOp>\n", indent, "");
        } break;
        case AST_COMPOUND_STATEMENT: {
            struct CompoundStatement *c = (struct CompoundStatement *) node;
            printf("%*s<CompoundStatement>\n", indent, "");
            indent += 2;
            for (int i = 0; i < c->statements.count; ++i) {
                struct AstNode *n = (struct AstNode *) List_Get(&c->statements, i);
                Print(n);
            }

            indent -= 2;
            printf("%*s</CompoundStatement>\n", indent, "");
        } break;
        case AST_FUNCTION_DEFINITION: {
            struct FunctionDefinition *f = (struct FunctionDefinition *) node;
            printf("%*s<FunctionDefinition stack_size=\"%d\">\n", indent, "", f->stack_size);
            indent += 2;
            for (int i = 0; i < f->statements.count; ++i) {
                struct AstNode *n = (struct AstNode *) List_Get(&f->statements, i);
                Print(n);
            }

            indent -= 2;
            printf("%*s</FunctionDefinition>\n", indent, "");
        } break;
        case AST_LITERAL_NUMBER: {
            struct Literal *l = (struct Literal *) node;
            printf("%*s<Literal int_value=\"%d\" operand=\"{ %d, %d }\" />\n", indent, "",
                l->int_value,
                l->operand.type,
                l->operand.base_type
            );
        } break;
        case AST_RETURN_STATEMENT: {
            struct ReturnStatement *r = (struct ReturnStatement *) node;
            printf("%*s<ReturnStatement>\n", indent, "");
            indent += 2;
            Print(r->expr);
            indent -= 2;
            printf("%*s</ReturnStatement>\n", indent, "");
        } break;
        case AST_UNARY_OPERATOR: {
            struct UnaryOp *u = (struct UnaryOp *) node;
            printf("%*s<UnaryOp operator_type=\"", indent, "");
            PrintOperatorType(u->operator_type);
            printf("\" operand=\"{ %d, %d }\" />\n",
                u->operand.type,
                u->operand.base_type
            );

            indent += 2;
            Print(u->expr);
            indent -= 2;
            printf("%*s</UnaryOp>\n", indent, "");
        } break;
        case AST_VARIABLE: {
            struct Variable *v = (struct Variable *) node;
            printf("%*s<Varaible identifier=\"%s\" operand=\"{ %d, %d }\" />\n", indent, "",
                v->identifier,
                v->operand.type,
                v->operand.base_type
            );
        } break;
        default: {
            printf("%*s<UNKNOWN type=\"%d\"/>\n", indent, "", node->type);
        } break;
    }
}
