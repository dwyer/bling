#include "bling/ast/ast.h"

extern object_t *object_new(obj_kind_t kind, char *name) {
    object_t obj = {
        .kind = kind,
        .name = name,
    };
    return memdup(&obj, sizeof(obj));
}

static const desc_t _object_desc = {
    .size = sizeof(object_t),
};

extern scope_t *scope_new(scope_t *outer) {
    scope_t *s = malloc(sizeof(scope_t));
    s->outer = outer;
    s->objects = make_map(&desc_str, &_object_desc);
    s->filenames = make_slice(sizeof(char *), 0, 0);
    return s;
}

extern void scope_deinit(scope_t *s) {
    map_deinit(&s->objects);
}

extern object_t *scope_insert(scope_t *s, object_t *obj) {
    object_t *alt = NULL;
    map_get(&s->objects, obj->name, &obj);
    if (alt == NULL) {
        map_set(&s->objects, obj->name, &obj);
    }
    return alt;
}

extern object_t *scope_lookup(scope_t *s, char *name) {
    object_t *obj = NULL;
    map_get(&s->objects, name, &obj);
    return obj;
}

typedef struct {
    scope_t *topScope;
} walker_t;

static expr_t *find_basic_type(token_t kind) {
    const char *name = NULL;
    switch (kind) {
    case token_INT:
        name = "int";
        break;
    case token_STRING:
        name = "string";
        break;
    default:
        panic("!");
    }
    expr_t x = {
        .type = ast_EXPR_IDENT,
        .ident = {
            .name = strdup(name),
        }
    };
    expr_t *y = memdup(&x, sizeof(expr_t));
    return y;
}

static expr_t *find_type(walker_t *w, expr_t *expr) {
    assert(expr);
    switch (expr->type) {
    case ast_EXPR_BASIC_LIT:
        return find_basic_type(expr->basic_lit.kind);
    case ast_EXPR_IDENT:
        break;
    default:
        panic("find_type: unknown expr: %d", expr->type);
        break;
    }
    return NULL;
}

extern void scope_declare(scope_t *s, obj_kind_t kind, expr_t *name) {
    assert(s != NULL);
    assert(name->type == ast_EXPR_IDENT);
    scope_insert(s, object_new(kind, name->ident.name));
}

static void walk_decl(walker_t *w, decl_t *decl) {
    switch (decl->type) {
    case ast_DECL_FUNC:
    case ast_DECL_IMPORT:
    case ast_DECL_TYPEDEF:
        break;
    case ast_DECL_VALUE:
        if (decl->value.type == NULL) {
            decl->value.type = find_type(w, decl->value.value);
        }
        assert(decl->value.name->type == ast_EXPR_IDENT);
        scope_declare(w->topScope, obj_kind_VALUE, decl->value.name);
        break;
    default:
        panic("walk_decl: not implemented: %d", decl->type);
    }
}

extern void walk_file(file_t *file) {
    walker_t w = {.topScope = file->scope};
    for (int i = 0; file->decls != NULL && file->decls[i] != NULL; i++) {
        walk_decl(&w, file->decls[i]);
    }
}
