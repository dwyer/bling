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

extern void scope_declare(scope_t *s, decl_t *decl) {
    obj_kind_t kind;
    expr_t *ident = NULL;
    switch (decl->type) {
    case ast_DECL_ENUM:
        kind = obj_kind_VALUE;
        ident = decl->enum_.name;
        break;
    case ast_DECL_FIELD:
        kind = obj_kind_VALUE;
        ident = decl->field.name;
        break;
    case ast_DECL_FUNC:
        kind = obj_kind_FUNC;
        ident = decl->func.name;
        break;
    case ast_DECL_NATIVE:
        kind = obj_kind_TYPE;
        ident = decl->native.name;
        break;
    case ast_DECL_TYPEDEF:
        kind = obj_kind_TYPE;
        ident = decl->typedef_.name;
        break;
    case ast_DECL_VALUE:
        kind = obj_kind_VALUE;
        ident = decl->value.name;
        break;
    default:
        panic("scope_declare: bad decl: %d", decl->type);
        return;
    }
    assert(ident->type == ast_EXPR_IDENT);
    if (ident->ident.obj != NULL) {
        decl_t *decl = ident->ident.obj->decl;
        panic("already declared: %s", ident->ident.name);
    }
    object_t *obj = object_new(kind, ident->ident.name);
    obj->decl = decl;
    ident->ident.obj = obj;
    object_t *alt = scope_insert(s, obj);
    if (alt != NULL) {
        bool warn = false;
        if (alt->kind == kind) {
            switch (kind) {
            case obj_kind_TYPE:
                warn = true;
                break;
            case obj_kind_VALUE:
                warn = alt->decl->value.value == NULL &&
                    decl->value.value != NULL;
                break;
            default:
                break;
            }
        }
        if (warn) {
            print("warning: already declared: %s", ident->ident.name);
        } else {
            panic("already declared: %s", ident->ident.name);
        }
    }
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
