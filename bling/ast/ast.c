#include "bling/ast/ast.h"
#include "bytes/bytes.h"

extern bool ast$isExprType(ast$Expr *x) {
    return ast$_TYPE_START < x->type && x->type < ast$_DECL_END;
}

extern ast$Object *ast$newObject(ast$ObjKind kind, char *name) {
    ast$Object obj = {
        .kind = kind,
        .name = name,
    };
    return esc(obj);
}

extern ast$Scope *ast$Scope_new(ast$Scope *outer) {
    ast$Scope s = {
        .outer = outer,
        .objects = utils$Map_init(sizeof(ast$Object *)),
    };
    return esc(s);
}

extern void ast$Scope_deinit(ast$Scope *s) {
    utils$Map_deinit(&s->objects);
}

extern ast$Object *ast$Scope_insert(ast$Scope *s, ast$Object *obj) {
    ast$Object *alt = NULL;
    utils$Map_get(&s->objects, obj->name, &alt);
    if (alt == NULL) {
        utils$Map_set(&s->objects, obj->name, &obj);
    }
    return alt;
}

extern ast$Object *ast$Scope_lookup(ast$Scope *s, char *name) {
    ast$Object *obj = NULL;
    utils$Map_get(&s->objects, name, &obj);
    return obj;
}

extern void ast$Scope_print(ast$Scope *s) {
    bytes$Buffer buf = {};
    while (s) {
        utils$Map_iter_t iter = utils$Map_iter(&s->objects);
        char *key = NULL;
        while (utils$Map_iter_next(&iter, &key, NULL)) {
            print("%s- %s", bytes$Buffer_string(&buf), key);
        }
        s = s->outer;
        bytes$Buffer_writeByte(&buf, '\t', NULL);
    }
}

extern void ast$Scope_resolve(ast$Scope *s, ast$Expr *x) {
    if (x->type != ast$EXPR_IDENT) {
        return;
    }
    assert(x->ident.obj == NULL);
    if (x->ident.pkg) {
        ast$Expr *pkg = x->ident.pkg;
        ast$Scope_resolve(s, pkg);
        if (pkg->ident.obj->kind != ast$ObjKind_PKG) {
            ast$Scope_print(s);
            panic("not a pkg: %s", pkg->ident.name);
        }
        ast$Decl *decl = pkg->ident.obj->decl;
        assert(decl->type == ast$DECL_IMPORT);
        ast$Scope *t = decl->imp.scope;
        if (!t) {
            ast$Scope_print(s);
            panic("%s $ %s", pkg->ident.name, x->ident.name);
        }
        assert(t);
        s = t;
    }
    for (ast$Scope *t = s; t != NULL; t = t->outer) {
        ast$Object *obj = ast$Scope_lookup(t, x->ident.name);
        if (obj != NULL) {
            x->ident.obj = obj;
            return;
        }
    }
    ast$Scope_print(s);
    panic("ast$Scope_resolve: unresolved: %s", x->ident.name);
}

extern void ast$Scope_free(ast$Scope *s) {
    utils$Map_deinit(&s->objects);
    free(s);
}

extern bool ast$isIdent(ast$Expr *x) {
    return x->type == ast$EXPR_IDENT;
}

extern bool ast$isIdentNamed(ast$Expr *x, const char *name) {
    return ast$isIdent(x) && streq(x->ident.name, name);
}

extern bool ast$isNil(ast$Expr *x) {
    return ast$isIdentNamed(x, "NULL");
}

extern bool ast$isVoid(ast$Expr *x) {
    return ast$isIdent(x) && streq(x->ident.name, "void");
}

extern bool ast$isVoidPtr(ast$Expr *x) {
    if (x->type == ast$EXPR_STAR) {
        return ast$isVoid(x->star.x);
    }
    return false;
}
