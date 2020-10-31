#ifndef MINIC_ASTNODES_H
#define MINIC_ASTNODES_H
#include "Common.h"


#define NEW_AST(type) (type *) malloc(sizeof(type));


enum AstType {
    AST_LITERAL_INT,
    AST_LITERAL_IDENT,
    AST_LITERAL_PTR,
    AST_LITERAL_DEREF,
    AST_BINARYOP,
    AST_VARDECL,
    AST_VARASSIGN,
    AST_BLOCK,
    AST_RETURNSTMT,
    AST_IFSTMT,
    AST_WHILELOOP,
    AST_FORLOOP,
    AST_FUNCDECL,
    AST_FUNCCALL,
    AST_FILE,
} typedef AstType;

enum OperatorType {
    OP_INVALID, // needed because 'op_precedences[0]' is -1
    OP_ADD, OP_SUB,
    OP_MUL, OP_DIV,
    OP_ISEQUAL, OP_NOTEQUAL,
    OP_ISLESS_THAN, OP_ISLESS_THAN_EQUAL,
    OP_ISGREATER_THAN, OP_ISGREATER_THAN_EQUAL
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

struct VarAssign {
    Ast info;
    char *ident;
    Ast *expr;
    Bool is_deref;
} typedef VarAssign;

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
    VarAssign *pre_operation;
    Ast *condition;
    VarAssign *post_operation;
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