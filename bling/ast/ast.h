#pragma once
#include "bling/token/token.h"
#include "utils/utils.h"

package(ast);

import("bling/token");
import("bytes");
import("sys");
import("utils");

typedef enum {

    ast$NODE_ILLEGAL = 0,

    ast$_DECL_START,
    ast$DECL_FIELD,
    ast$DECL_FUNC,
    ast$DECL_IMPORT,
    ast$DECL_PRAGMA,
    ast$DECL_TYPEDEF,
    ast$DECL_VALUE,
    ast$_DECL_END,

    ast$_EXPR_START,
    ast$EXPR_BASIC_LIT,
    ast$EXPR_BINARY,
    ast$EXPR_CALL,
    ast$EXPR_CAST,
    ast$EXPR_COMPOSITE_LIT,
    ast$EXPR_IDENT,
    ast$EXPR_INDEX,
    ast$EXPR_KEY_VALUE,
    ast$EXPR_PAREN,
    ast$EXPR_SELECTOR,
    ast$EXPR_SIZEOF,
    ast$EXPR_STAR,
    ast$EXPR_TERNARY,
    ast$EXPR_UNARY,
    ast$_EXPR_END,

    ast$_STMT_START,
    ast$STMT_ASSIGN,
    ast$STMT_BLOCK,
    ast$STMT_CASE,
    ast$STMT_DECL,
    ast$STMT_EMPTY,
    ast$STMT_EXPR,
    ast$STMT_IF,
    ast$STMT_ITER,
    ast$STMT_JUMP,
    ast$STMT_LABEL,
    ast$STMT_POSTFIX,
    ast$STMT_RETURN,
    ast$STMT_SWITCH,
    ast$_STMT_END,

    ast$_TYPE_START,
    ast$TYPE_ARRAY,
    ast$TYPE_BUILTIN_FUNC,
    ast$TYPE_ELLIPSIS,
    ast$TYPE_ENUM,
    ast$TYPE_NATIVE,
    ast$TYPE_FUNC,
    ast$TYPE_STRUCT,
    ast$_TYPE_END,

} ast$NodeKind;

typedef struct {
    ast$NodeKind kind;
} ast$Node;

typedef struct ast$Decl ast$Decl;
typedef struct ast$Expr ast$Expr;
typedef struct ast$Stmt ast$Stmt;

typedef struct ast$Scope ast$Scope;

typedef enum {
    ast$ObjKind_BAD,
    ast$ObjKind_CON,
    ast$ObjKind_FUN,
    // ast$ObjKind_LBL,
    ast$ObjKind_PKG,
    ast$ObjKind_TYP,
    ast$ObjKind_VAL,
} ast$ObjKind;

typedef struct {
    ast$ObjKind kind;
    char *name;
    ast$Decl *decl;
    void *data;
    void *type;
    ast$Scope *scope;
} ast$Object;

typedef struct {
    token$Pos pos;
    ast$Expr *len;
    ast$Expr *elt;
} ast$ArrayType;

typedef struct {
    token$Pos pos;
    token$Token kind;
    char *value;
} ast$BasicLit;

typedef struct {
    token$Token op;
    ast$Expr *x;
    ast$Expr *y;
} ast$BinaryExpr;

typedef struct {
    ast$Expr *func;
    ast$Expr **args;
} ast$CallExpr;

typedef struct {
    token$Pos pos;
    ast$Expr *type;
    ast$Expr *expr;
} ast$CastExpr;

typedef struct {
    token$Pos pos;
    ast$Expr *type;
    ast$Expr **list;
} ast$CompositeLit;

typedef struct {
    token$Pos pos;
} ast$EllipsisType;

typedef struct {
    token$Pos pos;
    ast$Expr *name;
    ast$Decl **enums;
} ast$EnumType;

typedef struct {
    token$Pos pos;
    ast$Expr *result;
    ast$Decl **params;
} ast$FuncExpr;

typedef struct {
    token$Pos pos;
    char *name;
    ast$Object *obj;
} ast$Ident;

typedef struct {
    ast$Expr *x;
    ast$Expr *index;
} ast$IndexExpr;

typedef struct {
    ast$Expr *key;
    ast$Expr *value;
    bool isArray;
} ast$KeyValueExpr;

typedef struct {
    char *name;
    int size;
} ast$NativeType;

typedef struct {
    token$Pos pos;
    ast$Expr *x;
} ast$ParenExpr;

typedef struct {
    ast$Expr *x;
    token$Token tok;
    ast$Expr *sel;
} ast$SelectorExpr;

typedef struct {
    token$Pos pos;
    ast$Expr *x;
} ast$SizeofExpr;

typedef struct {
    token$Pos pos;
    ast$Expr *x;
} ast$StarExpr;

typedef struct {
    token$Pos pos;
    token$Token tok;
    ast$Expr *name;
    ast$Decl **fields;
    ast$Scope *scope;
} ast$StructType;

typedef struct {
    ast$Expr *cond;
    ast$Expr *x;
    ast$Expr *y;
} ast$TernaryExpr;

typedef struct {
    token$Pos pos;
    token$Token op;
    ast$Expr *x;
} ast$UnaryExpr;

typedef struct ast$Expr {
    ast$Node;
    bool is_const;
    union {
        ast$ArrayType array;
        ast$BasicLit basic;
        ast$BinaryExpr binary;
        ast$CallExpr call;
        ast$CastExpr cast;
        ast$CompositeLit composite;
        ast$EllipsisType ellipsis;
        ast$EnumType enum_;
        ast$FuncExpr func;
        ast$Ident ident;
        ast$IndexExpr index;
        ast$KeyValueExpr key_value;
        ast$NativeType native;
        ast$ParenExpr paren;
        ast$SelectorExpr selector;
        ast$SizeofExpr sizeof_;
        ast$StarExpr star;
        ast$StructType struct_;
        ast$TernaryExpr ternary;
        ast$UnaryExpr unary;
    };
} ast$Expr;

typedef struct {
    ast$Expr *name;
    ast$Expr *path;
} ast$ImportDecl;

typedef struct {
    char *lit;
} ast$PragmaDecl;

typedef struct {
    ast$Expr *name;
    ast$Expr *type;
} ast$TypeDecl;

typedef struct {
    ast$Expr *name;
    ast$Expr *type;
    ast$Expr *value;
    token$Token kind;
} ast$ValueDecl;

typedef struct {
    ast$Expr *name;
    ast$Expr *type;
    ast$Stmt *body;
} ast$FuncDecl;

typedef struct {
    ast$Expr *name;
    ast$Expr *type;
} ast$Field;

typedef struct ast$Decl {
    ast$Node;
    token$Pos pos;
    union {
        ast$ImportDecl imp;
        ast$PragmaDecl pragma;
        ast$TypeDecl typedef_;
        ast$ValueDecl value;
        ast$FuncDecl func;
        ast$Field field;
    };
} ast$Decl;

typedef struct {
    ast$Expr *x;
    token$Token op;
    ast$Expr *y;
} ast$AssignStmt;

typedef struct {
    token$Pos pos;
    ast$Stmt **stmts;
} ast$BlockStmt;

typedef struct {
    token$Pos pos;
    ast$Expr **exprs;
    ast$Stmt **stmts;
} ast$CaseStmt;

typedef struct {
    token$Pos pos;
} ast$EmptyStmt;

typedef struct {
    ast$Expr *x;
} ast$ExprStmt;

typedef struct {
    ast$Decl *decl;
} ast$DeclStmt;

typedef struct {
    token$Pos pos;
    ast$Expr *cond;
    ast$Stmt *body;
    ast$Stmt *else_;
} ast$IfStmt;

typedef struct {
    token$Pos pos;
    token$Token kind;
    ast$Stmt *init;
    ast$Expr *cond;
    ast$Stmt *post;
    ast$Stmt *body;
} ast$IterStmt;

typedef struct {
    token$Pos pos;
    token$Token keyword;
    ast$Expr *label;
} ast$JumpStmt;

typedef struct {
    ast$Expr *label;
    ast$Stmt *stmt;
} ast$LabelStmt;

typedef struct {
    ast$Expr *x;
    token$Token op;
} ast$PostfixStmt;

typedef struct {
    token$Pos pos;
    ast$Expr *x;
} ast$ReturnStmt;

typedef struct {
    token$Pos pos;
    ast$Expr *tag;
    ast$Stmt **stmts;
} ast$SwitchStmt;

typedef struct ast$Stmt {
    ast$Node;
    union {
        ast$AssignStmt assign;
        ast$BlockStmt block;
        ast$CaseStmt case_;
        ast$DeclStmt decl;
        ast$EmptyStmt empty;
        ast$ExprStmt expr;
        ast$IterStmt iter;
        ast$IfStmt if_;
        ast$JumpStmt jump;
        ast$LabelStmt label;
        ast$PostfixStmt postfix;
        ast$ReturnStmt return_;
        ast$SwitchStmt switch_;
    };
} ast$Stmt;

extern ast$Object *ast$newObject(ast$ObjKind kind, char *name);

typedef struct ast$Scope {
    ast$Scope *outer;
    utils$Map objects;
    utils$Slice keys;
    char *pkg;
} ast$Scope;

extern bool ast$isExprType(ast$Expr *x);

extern ast$Scope *ast$Scope_new(ast$Scope *outer);
extern void ast$Scope_deinit(ast$Scope *s);
extern ast$Object *ast$Scope_insert(ast$Scope *s, ast$Object *obj);
extern ast$Object *ast$Scope_lookup(ast$Scope *s, char *name);

typedef struct {
    const char *filename;
    ast$Expr *name;
    ast$Decl **imports;
    ast$Decl **decls;
    ast$Scope *scope;
} ast$File;

typedef struct {
    char *name;
    ast$Scope *scope;
    ast$File **files;
} ast$Package;

extern void ast$Scope_free(ast$Scope *s);
extern void ast$Scope_print(ast$Scope *s);
extern bool ast$resolve(ast$Scope *scope, ast$Expr *ident);

extern bool ast$isIdent(ast$Expr *x);
extern bool ast$isIdentNamed(ast$Expr *x, const char *name);
extern bool ast$isNil(ast$Expr *x);
extern bool ast$isVoid(ast$Expr *x);
extern bool ast$isVoidPtr(ast$Expr *x);

extern token$Pos ast$Decl_pos(ast$Decl *x);
extern token$Pos ast$Expr_pos(ast$Expr *x);
extern token$Pos ast$Stmt_pos(ast$Stmt *x);
