#pragma once
#include "bling/token/token.h"
#include "map/map.h"

import("bling/token");
import("map");

typedef enum {

    ast_NODE_ILLEGAL = 0,

    _ast_DECL_START,
    ast_DECL_FIELD,
    ast_DECL_FUNC,
    ast_DECL_IMPORT,
    ast_DECL_PRAGMA,
    ast_DECL_TYPEDEF,
    ast_DECL_VALUE,
    _ast_DECL_END,

    _ast_EXPR_START,
    ast_EXPR_BASIC_LIT,
    ast_EXPR_BINARY,
    ast_EXPR_CALL,
    ast_EXPR_CAST,
    ast_EXPR_COMPOUND,
    ast_EXPR_COND,
    ast_EXPR_IDENT,
    ast_EXPR_INDEX,
    ast_EXPR_INIT_DECL,
    ast_EXPR_KEY_VALUE,
    ast_EXPR_PAREN,
    ast_EXPR_SELECTOR,
    ast_EXPR_SIZEOF,
    ast_EXPR_STAR,
    ast_EXPR_UNARY,
    _ast_EXPR_END,

    _ast_STMT_START,
    ast_STMT_ASSIGN,
    ast_STMT_BLOCK,
    ast_STMT_CASE,
    ast_STMT_DECL,
    ast_STMT_EMPTY,
    ast_STMT_EXPR,
    ast_STMT_IF,
    ast_STMT_ITER,
    ast_STMT_JUMP,
    ast_STMT_LABEL,
    ast_STMT_POSTFIX,
    ast_STMT_RETURN,
    ast_STMT_SWITCH,
    _ast_STMT_END,

    _ast_TYPE_START,
    ast_TYPE_ARRAY,
    ast_TYPE_ENUM,
    ast_TYPE_NATIVE,
    ast_TYPE_FUNC,
    ast_TYPE_STRUCT,
    _ast_TYPE_END,

} ast_node_type_t;

typedef struct decl_t decl_t;
typedef struct expr_t expr_t;
typedef struct stmt_t stmt_t;

typedef enum {
    obj_kind_FUNC,
    obj_kind_PKG,
    obj_kind_TYPE,
    obj_kind_VALUE,
} obj_kind_t;

typedef struct {
    obj_kind_t kind;
    char *name;
    decl_t *decl;
    char *pkg;
} ast_Object;

typedef struct decl_t {
    ast_node_type_t type;
    token$Pos pos;
    union {

        struct {
            expr_t *name;
            expr_t *path;
        } imp;

        struct {
            char *lit;
        } pragma;

        struct {
            expr_t *name;
            expr_t *type;
        } typedef_;

        struct {
            expr_t *name;
            expr_t *type;
            expr_t *value;
            token$Token kind;
        } value;

        struct {
            expr_t *name;
            expr_t *type;
            stmt_t *body;
        } func;

        struct {
            expr_t *name;
            expr_t *type;
        } field;

    };
} decl_t;

typedef struct expr_t {
    ast_node_type_t type;
    token$Pos pos;
    bool is_const;

    union {

        struct {
            expr_t *len;
            expr_t *elt;
        } array;

        struct {
            token$Token kind;
            char *value;
        } basic_lit;

        struct {
            token$Token op;
            expr_t *x;
            expr_t *y;
        } binary;

        struct {
            expr_t *func;
            expr_t **args;
        } call;

        struct {
            expr_t *type;
            expr_t *expr;
        } cast;

        struct {
            expr_t *type;
            expr_t **list;
        } compound;

        struct {
            expr_t *condition;
            expr_t *consequence;
            expr_t *alternative;
        } conditional;

        struct {
            expr_t *name;
            decl_t **enums;
        } enum_;

        struct {
            expr_t *result;
            decl_t **params;
        } func;

        struct {
            char *name;
            ast_Object *obj;
        } ident;

        struct {
            expr_t *x;
            expr_t *index;
        } index;

        struct {
            expr_t *key;
            expr_t *value;
            bool isArray;
        } key_value;

        struct {
            char *name;
            int size;
        } native;

        struct {
            expr_t *x;
        } paren;

        struct {
            expr_t *x;
            token$Token tok;
            expr_t *sel;
        } selector;

        struct {
            expr_t *x;
        } sizeof_;

        struct {
            expr_t *x;
        } star;

        struct {
            token$Token tok;
            expr_t *name;
            decl_t **fields;
        } struct_;

        struct {
            token$Token op;
            expr_t *x;
        } unary;

    };
} expr_t;

typedef struct stmt_t {
    ast_node_type_t type;
    token$Pos pos;
    union {

        struct {
            expr_t *x;
            token$Token op;
            expr_t *y;
        } assign;

        struct {
            stmt_t **stmts;
        } block;

        struct {
            expr_t **exprs;
            stmt_t **stmts;
        } case_;

        decl_t *decl;

        struct {
            expr_t *x;
        } expr;

        struct {
            token$Token kind;
            stmt_t *init;
            expr_t *cond;
            stmt_t *post;
            stmt_t *body;
        } iter;

        struct {
            expr_t *cond;
            stmt_t *body;
            stmt_t *else_;
        } if_;

        struct {
            token$Token keyword;
            expr_t *label;
        } jump;

        struct {
            expr_t *label;
            stmt_t *stmt;
        } label;

        struct {
            expr_t *x;
            token$Token op;
        } postfix;

        struct {
            expr_t *x;
        } return_;

        struct {
            expr_t *tag;
            stmt_t **stmts;
        } switch_;

    };
} stmt_t;

extern ast_Object *object_new(obj_kind_t kind, char *name);

typedef struct ast_Scope ast_Scope;

typedef struct ast_Scope {
    ast_Scope *outer;
    map$Map objects;
    char *pkg;
} ast_Scope;

extern bool is_expr_type(expr_t *x);

extern ast_Scope *scope_new(ast_Scope *outer);
extern void scope_deinit(ast_Scope *s);
extern ast_Object *scope_insert(ast_Scope *s, ast_Object *obj);
extern ast_Object *scope_lookup(ast_Scope *s, char *name);

typedef struct {
    const char *filename;
    expr_t *name;
    decl_t **imports;
    decl_t **decls;
    ast_Scope *scope;
} ast_File;

typedef struct {
    char *name;
    ast_Scope *scope;
    ast_File **files;
} package_t;

extern void scope_resolve(ast_Scope *s, expr_t *x);
extern void scope_free(ast_Scope *s);

extern bool ast_isIdent(expr_t *x);
extern bool ast_isIdentNamed(expr_t *x, const char *name);
extern bool ast_isNil(expr_t *x);
extern bool ast_isVoid(expr_t *x);
extern bool ast_isVoidPtr(expr_t *x);
