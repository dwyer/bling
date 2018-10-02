typedef enum {

    ast_NODE_ILLEGAL = 0,

    ast_DECL_FUNC,
    ast_DECL_GEN,

    ast_EXPR_BASIC_LIT,
    ast_EXPR_BINARY,
    ast_EXPR_CALL,
    ast_EXPR_IDENT,
    ast_EXPR_INCDEC,
    ast_EXPR_INDEX,
    ast_EXPR_UNARY,
    ast_EXPR_SELECTOR,
    ast_EXPR_INIT_DECL,

    ast_SPEC_TYPEDEF,
    ast_SPEC_VALUE,

    ast_STMT_BLOCK,
    ast_STMT_DECL,
    ast_STMT_EXPR,
    ast_STMT_IF,
    ast_STMT_RETURN,
    ast_STMT_WHILE,

    ast_TYPE_ARRAY,
    ast_TYPE_ENUM,
    ast_TYPE_PTR,
    ast_TYPE_FUNC,
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
} field_t;

struct decl {
    ast_node_type_t type;
    union {

        struct {
            spec_t *spec;
            // TODO: spec_t **specs;
        } gen;

        struct {
            expr_t *type;
            expr_t *name;
            stmt_t *body;
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
            int kind;
            char *value;
        } basic_lit;

        struct {
            int op;
            expr_t *x;
            expr_t *y;
        } binary;

        struct {
            expr_t *func;
            expr_t **args;
        } call;

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
            int tok;
        } incdec;

        struct {
            expr_t *x;
            expr_t *index;
        } index;

        struct {
            expr_t *type;
        } ptr;

        struct {
            expr_t *x;
            expr_t *sel;
        } selector;

        struct {
            int tok;
            expr_t *name;
            field_t **fields;
        } struct_;

        struct {
            int op;
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

        decl_t *decl;

        struct {
            expr_t *x;
        } expr;

        struct {
            expr_t *cond;
            stmt_t *body;
            stmt_t *else_;
        } if_;

        struct {
            expr_t *x;
        } return_;

        struct {
            expr_t *cond;
            stmt_t *body;
        } while_;

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
