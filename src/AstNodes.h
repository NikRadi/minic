#ifndef MINIC_ASTNODES_H
#define MINIC_ASTNODES_H
#include "Common.h"


#define NEW_AST(type) (type *) malloc(sizeof(type));


enum AstType {
#define AST(name, str) AST_##name,
#include "AstNodes.def"
} typedef AstType;
char *GetAstTypeStr(AstType type);


enum OperatorType {
    OP_INVALID,
    BIOP_ISEQUAL,    BIOP_NOTEQUAL,
    BIOP_LESS,       BIOP_LESS_EQUAL,
    BIOP_GREATER,    BIOP_GREATER_EQUAL,
    BIOP_ADD,        BIOP_SUB,
    BIOP_MUL,        BIOP_DIV,
    UNOP_DEREF,      UNOP_ADDRESS,
    ASOP_EQUAL,
    ASOP_ADD_EQUAL, ASOP_SUB_EQUAL,
    ASOP_MUL_EQUAL, ASOP_DIV_EQUAL,
} typedef OperatorType;

enum DataType {
    DATA_INT, DATA_CHAR, DATA_VOID,
    DATA_INT_PTR, DATA_CHAR_PTR,
    DATA_INT_ARR, DATA_CHAR_ARR,
} typedef DataType;

struct Ast {
    AstType type;
    struct Ast *parent;
} typedef Ast;

struct Literal {
    Ast info;
    union {
        int intvalue;
        struct {
            char *strvalue;
            int arridx;
        };
    };
} typedef Literal;

struct UnaryOp {
    Ast info;
    OperatorType optype;
    Ast *expr;
} typedef UnaryOp;

struct BinaryOp {
    Ast info;
    OperatorType optype;
    Ast *lhs;
    Ast *rhs;
} typedef BinaryOp;

struct VarDecl {
    Ast info;
    DataType datatype;
    char *ident;
    Ast *expr;
    int arrsize;
} typedef VarDecl;

struct Block {
    Ast info;
    Ast *stmt;
    struct Block *glue;
} typedef Block;

struct IfStmt {
    Ast info;
    Ast *condition;
    Block *block;
    struct IfStmt *elsestmt;
} typedef IfStmt;

struct ReturnStmt {
    Ast info;
    Ast *expr;
} typedef ReturnStmt;

struct WhileLoop {
    Ast info;
    Ast *condition;
    Block *block;
} typedef WhileLoop;

struct ForLoop {
    Ast info;
    BinaryOp *pre_operation;
    Ast *condition;
    BinaryOp *post_operation;
    Block *block;
} typedef ForLoop;

struct FuncCall {
    Ast info;
    char *ident;
    Ast *arg;
} typedef FuncCall;

struct FuncDecl {
    Ast info;
    int stack_depth_bytes;
    DataType returntype;
    char *ident;
    Block *block;
} typedef FuncDecl;

struct File {
    Ast info;
    FuncDecl *funcdecl;
    struct File *glue;
} typedef File;


#endif // MINIC_ASTNODES_H