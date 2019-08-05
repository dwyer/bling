#include "bling/ast/ast.h"

extern bool is_expr_type(expr_t *x) {
    return _ast_TYPE_START < x->type && x->type < _ast_DECL_END;
}

extern ast_Object *object_new(obj_kind_t kind, char *name) {
    ast_Object obj = {
        .kind = kind,
        .name = name,
    };
    return esc(obj);
}

extern ast_Scope *scope_new(ast_Scope *outer) {
    ast_Scope s = {
        .outer = outer,
        .objects = map_init(sizeof(ast_Object *)),
    };
    return esc(s);
}

extern void scope_deinit(ast_Scope *s) {
    map_deinit(&s->objects);
}

extern ast_Object *scope_insert(ast_Scope *s, ast_Object *obj) {
    ast_Object *alt = NULL;
    map_get(&s->objects, obj->name, &alt);
    if (alt == NULL) {
        map_set(&s->objects, obj->name, &obj);
    }
    return alt;
}

extern ast_Object *scope_lookup(ast_Scope *s, char *name) {
    ast_Object *obj = NULL;
    map_get(&s->objects, name, &obj);
    return obj;
}

extern void scope_resolve(ast_Scope *s, expr_t *x) {
    if (x->type != ast_EXPR_IDENT) {
        return;
    }
    assert(x->ident.obj == NULL);
    while (s != NULL) {
        ast_Object *obj = scope_lookup(s, x->ident.name);
        if (obj != NULL) {
            x->ident.obj = obj;
            return;
        }
        s = s->outer;
    }
    panic("scope_resolve: unresolved: %s", x->ident.name);
}

extern void scope_free(ast_Scope *s) {
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
    return ast_isIdent(x) && streq(x->ident.name, "void");
}

extern bool ast_isVoidPtr(expr_t *x) {
    if (x->type == ast_EXPR_STAR) {
        return ast_isVoid(x->star.x);
    }
    return false;
}
