#ifndef MINIC_ASTNODES_H
#define MINIC_ASTNODES_H
#include "Common.h"
#include "List.h"


enum AstType {
#define AST(name, str) AST_##name,
#include "AstTypes.def"
} typedef AstType;

enum OperatorType {
#define OP(name) name,
#include "OperatorTypes.def"
} typedef OperatorType;

enum DataType {
    DATA_VOID, DATA_CHAR, DATA_INT, DATA_LONG, DATA_STRUCT
} typedef DataType;

enum StorageType {
    STOR_GLOBAL, STOR_LOCAL, STOR_PARAM
} typedef StorageType;

struct VarData {
    DataType datatype;
    StorageType storagetype;
    char *ident;
    int mem_location;
    int lvl_indirection;
} typedef VarData;

struct Scope {
    struct Scope *parent;
    VarData vardatas[16];
    int num_vars;
} typedef Scope;

struct Ast {
    AstType type;
} typedef Ast;

struct Literal {
    Ast info;
    int intvalue;
    char *strvalue;
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
    char *ident;
    DataType datatype;
    int arrsize;
    int lvl_indirection;
    Ast *expr;
} typedef VarDecl;

struct Block {
    Ast info;
    List stmts;
    Scope scope;
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
    List args;
} typedef FuncCall;

struct FuncDecl {
    Ast info;
    char *ident;
    int stack_depth_bytes;
    DataType returntype;
    List params;
    Block *block;
} typedef FuncDecl;

struct File {
    Ast info;
    char *name;
    List decls;
} typedef File;


#endif // MINIC_ASTNODES_H