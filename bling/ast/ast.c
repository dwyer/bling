#include "bling/ast/ast.h"

extern bool is_expr_type(expr_t *x) {
    return _ast_TYPE_START < x->type && x->type < _ast_DECL_END;
}

extern object_t *object_new(obj_kind_t kind, char *name) {
    object_t obj = {
        .kind = kind,
        .name = name,
    };
    return esc(obj);
}

extern scope_t *scope_new(scope_t *outer) {
    scope_t s = {
        .outer = outer,
        .objects = map_init(sizeof(object_t)),
        .filenames = make_slice(sizeof(char *), 0, 0),
    };
    return esc(s);
}

extern void scope_deinit(scope_t *s) {
    map_deinit(&s->objects);
}

extern object_t *scope_insert(scope_t *s, object_t *obj) {
    object_t *alt = NULL;
    map_get(&s->objects, obj->name, &alt);
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

extern void scope_resolve(scope_t *s, expr_t *x) {
    if (x->type != ast_EXPR_IDENT) {
        return;
    }
    assert(x->ident.obj == NULL);
    while (s != NULL) {
        object_t *obj = scope_lookup(s, x->ident.name);
        if (obj != NULL) {
            x->ident.obj = obj;
            return;
        }
        s = s->outer;
    }
    panic("scope_resolve: unresolved: %s", x->ident.name);
}

extern void scope_free(scope_t *s) {
    slice_deinit(&s->filenames);
    map_deinit(&s->objects);
    free(s);
}

extern bool ast_isIdent(expr_t *x) {
    return x->type == ast_EXPR_IDENT;
}

extern bool ast_isIdentNamed(expr_t *x, const char *name) {
    return ast_isIdent(x) && streq(x->ident.name, name);
}

extern bool ast_isNil(expr_t *x) {
    return ast_isIdentNamed(x, "NULL");
}

extern bool ast_isVoid(expr_t *x) {
    if (x->type == ast_TYPE_QUAL) {
        x = x->qual.type;
    }
    return ast_isIdent(x) && streq(x->ident.name, "void");
}

extern bool ast_isVoidPtr(expr_t *x) {
    if (x->type == ast_EXPR_STAR) {
        return ast_isVoid(x->star.x);
    }
    return false;
}
