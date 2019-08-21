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
        .keys = utils$Slice_init(sizeof(char *)),
    };
    return esc(s);
}

extern void ast$Scope_deinit(ast$Scope *s) {
    if (s) {
        utils$Map_deinit(&s->objects);
        utils$Slice_deinit(&s->keys);
    }
}

extern ast$Object *ast$Scope_insert(ast$Scope *s, ast$Object *obj) {
    ast$Object *alt = NULL;
    utils$Map_get(&s->objects, obj->name, &alt);
    if (alt == NULL) {
        utils$Map_set(&s->objects, obj->name, &obj);
        utils$Slice_append(&s->keys, &obj->name);
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

extern token$Pos ast$Decl_pos(ast$Decl *d) {
    return d->pos;
}

extern token$Pos ast$Expr_pos(ast$Expr *x) {
    switch (x->kind) {
    case ast$EXPR_BASIC_LIT: return x->basic.pos;
    case ast$EXPR_BINARY: return ast$Expr_pos(x->binary.x);
    case ast$EXPR_CALL: return ast$Expr_pos(x->call.func);
    case ast$EXPR_CAST: return x->cast.pos;
    case ast$EXPR_COMPOSITE_LIT: return x->composite.pos;
    case ast$EXPR_IDENT: return x->ident.pos;
    case ast$EXPR_INDEX: return ast$Expr_pos(x->index.x);
    case ast$EXPR_KEY_VALUE: return ast$Expr_pos(x->key_value.key);
    case ast$EXPR_PAREN: return x->paren.pos;
    case ast$EXPR_SELECTOR: return ast$Expr_pos(x->selector.x);
    case ast$EXPR_SIZEOF: return x->sizeof_.pos;
    case ast$EXPR_STAR: return x->star.pos;
    case ast$EXPR_TERNARY: return ast$Expr_pos(x->ternary.cond);
    case ast$EXPR_UNARY: return x->unary.pos;
    case ast$TYPE_ARRAY: return x->array.pos;
    case ast$TYPE_ENUM: return x->enum_.pos;
    case ast$TYPE_NATIVE: return 0;
    case ast$TYPE_FUNC: return x->func.pos;
    case ast$TYPE_STRUCT: return x->struct_.pos;
    default: return 0;
    }
}

extern token$Pos ast$Stmt_pos(ast$Stmt *s) {
    switch (s->kind) {
    case ast$STMT_ASSIGN: return ast$Expr_pos(s->assign.x);
    case ast$STMT_BLOCK: return s->block.pos;
    case ast$STMT_CASE: return s->case_.pos;
    case ast$STMT_DECL: return ast$Decl_pos(s->decl.decl);
    case ast$STMT_EMPTY: return s->empty.pos;
    case ast$STMT_EXPR: return ast$Expr_pos(s->expr.x);
    case ast$STMT_IF: return s->if_.pos;
    case ast$STMT_ITER: return s->iter.pos;
    case ast$STMT_JUMP: return s->jump.pos;
    case ast$STMT_LABEL: return ast$Expr_pos(s->label.label);
    case ast$STMT_POSTFIX: return ast$Expr_pos(s->postfix.x);
    case ast$STMT_RETURN: return s->return_.pos;
    case ast$STMT_SWITCH: return s->switch_.pos;
    default: return 0;
    }
}
