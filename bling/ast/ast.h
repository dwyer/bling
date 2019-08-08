#pragma once
#include "bling/token/token.h"
#include "utils/utils.h"

package(ast);

import("bling/token");
import("bytes");
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
    ast$EXPR_COMPOUND,
    ast$EXPR_COND,
    ast$EXPR_IDENT,
    ast$EXPR_INDEX,
    ast$EXPR_INIT_DECL,
    ast$EXPR_KEY_VALUE,
    ast$EXPR_PAREN,
    ast$EXPR_SELECTOR,
    ast$EXPR_SIZEOF,
    ast$EXPR_STAR,
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
    ast$TYPE_ENUM,
    ast$TYPE_NATIVE,
    ast$TYPE_FUNC,
    ast$TYPE_STRUCT,
    ast$_TYPE_END,

} ast$NodeType;

typedef struct ast$Decl ast$Decl;
typedef struct ast$Expr ast$Expr;
typedef struct ast$Scope ast$Scope;
typedef struct ast$Stmt ast$Stmt;

typedef enum {
    ast$ObjKind_FUNC,
    ast$ObjKind_PKG,
    ast$ObjKind_TYPE,
    ast$ObjKind_VALUE,
} ast$ObjKind;

typedef struct {
    ast$ObjKind kind;
    char *name;
    ast$Decl *decl;
    char *pkg;
} ast$Object;

typedef struct ast$Decl {
    ast$NodeType type;
    token$Pos pos;
    union {

        struct {
            ast$Expr *name;
            ast$Expr *path;
            ast$Scope *scope;
        } imp;

        struct {
            char *lit;
        } pragma;

        struct {
            ast$Expr *name;
            ast$Expr *type;
        } typedef_;

        struct {
            ast$Expr *name;
            ast$Expr *type;
            ast$Expr *value;
            token$Token kind;
        } value;

        struct {
            ast$Expr *name;
            ast$Expr *type;
            ast$Stmt *body;
        } func;

        struct {
            ast$Expr *name;
            ast$Expr *type;
        } field;

    };
} ast$Decl;

typedef struct ast$Expr {
    ast$NodeType type;
    token$Pos pos;
    bool is_const;

    union {

        struct {
            ast$Expr *len;
            ast$Expr *elt;
        } array;

        struct {
            token$Token kind;
            char *value;
        } basic_lit;

        struct {
            token$Token op;
            ast$Expr *x;
            ast$Expr *y;
        } binary;

        struct {
            ast$Expr *func;
            ast$Expr **args;
        } call;

        struct {
            ast$Expr *type;
            ast$Expr *expr;
        } cast;

        struct {
            ast$Expr *type;
            ast$Expr **list;
        } compound;

        struct {
            ast$Expr *condition;
            ast$Expr *consequence;
            ast$Expr *alternative;
        } conditional;

        struct {
            ast$Expr *name;
            ast$Decl **enums;
        } enum_;

        struct {
            ast$Expr *result;
            ast$Decl **params;
        } func;

        struct {
            char *name;
            ast$Object *obj;
            ast$Expr *pkg;
        } ident;

        struct {
            ast$Expr *x;
            ast$Expr *index;
        } index;

        struct {
            ast$Expr *key;
            ast$Expr *value;
            bool isArray;
        } key_value;

        struct {
            char *name;
            int size;
        } native;

        struct {
            ast$Expr *x;
        } paren;

        struct {
            ast$Expr *x;
            token$Token tok;
            ast$Expr *sel;
        } selector;

        struct {
            ast$Expr *x;
        } sizeof_;

        struct {
            ast$Expr *x;
        } star;

        struct {
            token$Token tok;
            ast$Expr *name;
            ast$Decl **fields;
        } struct_;

        struct {
            token$Token op;
            ast$Expr *x;
        } unary;

    };
} ast$Expr;

typedef struct ast$Stmt {
    ast$NodeType type;
    token$Pos pos;
    union {

        struct {
            ast$Expr *x;
            token$Token op;
            ast$Expr *y;
        } assign;

        struct {
            ast$Stmt **stmts;
        } block;

        struct {
            ast$Expr **exprs;
            ast$Stmt **stmts;
        } case_;

        ast$Decl *decl;

        struct {
            ast$Expr *x;
        } expr;

        struct {
            token$Token kind;
            ast$Stmt *init;
            ast$Expr *cond;
            ast$Stmt *post;
            ast$Stmt *body;
        } iter;

        struct {
            ast$Expr *cond;
            ast$Stmt *body;
            ast$Stmt *else_;
        } if_;

        struct {
            token$Token keyword;
            ast$Expr *label;
        } jump;

        struct {
            ast$Expr *label;
            ast$Stmt *stmt;
        } label;

        struct {
            ast$Expr *x;
            token$Token op;
        } postfix;

        struct {
            ast$Expr *x;
        } return_;

        struct {
            ast$Expr *tag;
            ast$Stmt **stmts;
        } switch_;

    };
} ast$Stmt;

extern ast$Object *ast$newObject(ast$ObjKind kind, char *name);

typedef struct ast$Scope {
    ast$Scope *outer;
    utils$Map objects;
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

extern void ast$Scope_resolve(ast$Scope *s, ast$Expr *x);
extern void ast$Scope_free(ast$Scope *s);

extern bool ast$isIdent(ast$Expr *x);
extern bool ast$isIdentNamed(ast$Expr *x, const char *name);
extern bool ast$isNil(ast$Expr *x);
extern bool ast$isVoid(ast$Expr *x);
extern bool ast$isVoidPtr(ast$Expr *x);

extern void ast$Scope_print(ast$Scope *s);
