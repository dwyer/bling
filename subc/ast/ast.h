#pragma once
#include "builtin/builtin.h"
#include "subc/token/token.h"

typedef enum {

    ast_NODE_ILLEGAL = 0,

    ast_DECL_FUNC,
    ast_DECL_GEN,

    ast_EXPR_BASIC_LIT,
    ast_EXPR_BINARY,
    ast_EXPR_CALL,
    ast_EXPR_CAST,
    ast_EXPR_COMPOUND,
    ast_EXPR_COND,
    ast_EXPR_IDENT,
    ast_EXPR_INCDEC,
    ast_EXPR_INDEX,
    ast_EXPR_KEY_VALUE,
    ast_EXPR_PAREN,
    ast_EXPR_UNARY,
    ast_EXPR_SELECTOR,
    ast_EXPR_SIZEOF,
    ast_EXPR_INIT_DECL,

    ast_SPEC_STORAGE,
    ast_SPEC_TYPEDEF,
    ast_SPEC_VALUE,

    ast_STMT_BLOCK,
    ast_STMT_CASE,
    ast_STMT_DECL,
    ast_STMT_EXPR,
    ast_STMT_IF,
    ast_STMT_ITER,
    ast_STMT_JUMP,
    ast_STMT_LABEL,
    ast_STMT_RETURN,
    ast_STMT_SWITCH,

    ast_TYPE_ARRAY,
    ast_TYPE_ENUM,
    ast_TYPE_NAME,
    ast_TYPE_PTR,
    ast_TYPE_FUNC,
    ast_TYPE_QUAL,
    ast_TYPE_STRUCT,

} ast_node_type_t;

typedef struct decl decl_t;
typedef struct expr expr_t;
typedef struct spec spec_t;
typedef struct stmt stmt_t;

typedef struct {
    expr_t *name;
    expr_t *value;
} enumerator_t;

typedef struct {
    expr_t *type;
    expr_t *name;
    bool is_const;
} field_t;

struct decl {
    ast_node_type_t type;
    token_t store;
    union {

        struct {
            spec_t *spec;
            // TODO: spec_t **specs;
            token_t store;
        } gen;

        struct {
            expr_t *type;
            expr_t *name;
            stmt_t *body;
            token_t store;
        } func;

    };
};

struct expr {
    ast_node_type_t type;
    union {

        struct {
            expr_t *len;
            expr_t *elt;
        } array;

        struct {
            token_t kind;
            char *value;
        } basic_lit;

        struct {
            token_t op;
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
            expr_t **list;
        } compound;

        struct {
            expr_t *condition;
            expr_t *consequence;
            expr_t *alternative;
        } conditional;

        struct {
            expr_t *name;
            enumerator_t **enumerators;
        } enum_;

        struct {
            expr_t *result;
            field_t **params;
        } func;

        struct {
            char *name;
        } ident;

        struct {
            expr_t *x;
            token_t tok;
        } incdec;

        struct {
            expr_t *x;
            expr_t *index;
        } index;

        struct {
            expr_t *key;
            expr_t *value;
        } key_value;

        struct {
            expr_t *x;
        } paren;

        struct {
            expr_t *type;
        } ptr;

        struct {
            expr_t *x;
            token_t tok;
            expr_t *sel;
        } selector;

        struct {
            expr_t *x;
        } sizeof_;

        struct {
            token_t tok;
            expr_t *name;
            field_t **fields;
        } struct_;

        struct {
            expr_t *type;
        } type_name;

        struct {
            token_t qual;
            expr_t *type;
        } qual;

        struct {
            token_t op;
            expr_t *x;
        } unary;

    };
};

struct stmt {
    ast_node_type_t type;
    union {

        struct {
            stmt_t **stmts;
        } block;

        struct {
            expr_t *expr;
            stmt_t **stmts;
        } case_;

        decl_t *decl;

        struct {
            expr_t *x;
        } expr;

        struct {
            token_t kind;
            stmt_t *init;
            expr_t *cond;
            expr_t *post;
            stmt_t *body;
        } iter;

        struct {
            expr_t *cond;
            stmt_t *body;
            stmt_t *else_;
        } if_;

        struct {
            token_t keyword;
            expr_t *label;
        } jump;

        struct {
            expr_t *label;
        } label;

        struct {
            expr_t *x;
        } return_;

        struct {
            expr_t *tag;
            stmt_t **stmts;
        } switch_;

    };
};

struct spec {
    ast_node_type_t type;
    union {

        struct {
            expr_t *type;
            expr_t *name;
        } typedef_;

        struct {
            expr_t *type;
            expr_t *name;
            expr_t *value;
        } value;

    };
};

typedef enum {
    obj_kind_BAD,
    obj_kind_TYPE,
} obj_kind_t;

typedef struct {
    obj_kind_t kind;
    char *name;
} object_t;

typedef struct scope scope_t;

struct scope {
    scope_t *outer;
    map_t objects;
};

extern scope_t *scope_new(scope_t *outer);
extern void scope_deinit(scope_t *s);
extern object_t *scope_insert(scope_t *s, object_t *obj);
extern object_t *scope_lookup(scope_t *s, char *name);

typedef struct {
    const char *filename;
    expr_t *name;
    decl_t **decls;
    scope_t *scope;
} file_t;

typedef struct {
    char *name;
    scope_t *scope;
    slice_t files;
} package_t;