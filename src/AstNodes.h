#ifndef MINIC_ASTNODES_H
#define MINIC_ASTNODES_H
#include "Common.h"
#include "List2Links.h"


#define NEW_AST(type) (type *) malloc(sizeof(type));


enum AstType {
#define AST(name, str) AST_##name,
#include "AstNodes.def"
} typedef AstType;
char *GetAstTypeStr(AstType type);


enum OperatorType {
    // Parser.c::op_precedences depends on the order of these
    OP_INVALID,
    BIOP_AND,        BIOP_OR,
    BIOP_ISEQUAL,    BIOP_NOTEQUAL,
    BIOP_LESS,       BIOP_LESS_EQUAL,
    BIOP_GREATER,    BIOP_GREATER_EQUAL,
    BIOP_ADD,        BIOP_SUB,
    BIOP_MUL,        BIOP_DIV,

    UNOP_DEREF,      UNOP_ADDRESS,
    UNOP_NOT,
    ASOP_EQUAL,
    ASOP_ADD_EQUAL,  ASOP_SUB_EQUAL,
    ASOP_MUL_EQUAL,  ASOP_DIV_EQUAL,
} typedef OperatorType;

enum DataType {
    DATA_INT, DATA_CHAR, DATA_VOID,
    DATA_INT_PTR, DATA_CHAR_PTR,
    DATA_INT_ARR, DATA_CHAR_ARR,
} typedef DataType;

struct Ast {
    AstType type;
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
    List2Links stmts;
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
    List2Links args;
} typedef FuncCall;

struct FuncDecl {
    Ast info;
    int stack_depth_bytes;
    List2Links params;
    DataType returntype;
    char *ident;
    Block *block;
} typedef FuncDecl;

struct File {
    Ast info;
    List2Links funcdecls;
} typedef File;


#endif // MINIC_ASTNODES_H