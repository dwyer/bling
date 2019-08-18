#include "bling/ast/ast.h"
#include "bytes/bytes.h"
#include "sys/sys.h"

extern bool ast$isExprType(ast$Expr *x) {
    return ast$_TYPE_START < x->kind && x->kind < ast$_DECL_END;
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
        utils$MapIter iter = utils$NewMapIter(&s->objects);
        char *key = NULL;
        while (utils$MapIter_next(&iter, &key, NULL)) {
            char *s = sys$sprintf("%s- %s", bytes$Buffer_string(&buf), key);
            print(s);
            free(s);
        }
        s = s->outer;
        bytes$Buffer_writeByte(&buf, '\t', NULL);
    }
}

extern bool ast$resolve(ast$Scope *scope, ast$Expr *ident) {
    for (; scope != NULL; scope = scope->outer) {
        ast$Object *obj = ast$Scope_lookup(scope, ident->ident.name);
        if (obj != NULL) {
            ident->ident.obj = obj;
            return true;
        }
    }
    return false;
}

extern void ast$Scope_free(ast$Scope *s) {
    utils$Map_deinit(&s->objects);
    free(s);
}

extern bool ast$isIdent(ast$Expr *x) {
    return x->kind == ast$EXPR_IDENT;
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
    if (x->kind == ast$EXPR_STAR) {
        return ast$isVoid(x->star.x);
    }
    return false;
}
