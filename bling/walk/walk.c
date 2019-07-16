#include "bling/ast/ast.h"

static expr_t *find_basic_type(token_t kind) {
    const char *name = NULL;
    switch (kind) {
    case token_INT:
        name = "int";
        break;
    case token_FLOAT:
        name = "float";
        break;
    default:
        panic("find_basic_type: bad kind: %s", token_string(kind));
    }
    expr_t x = {
        .type = ast_EXPR_IDENT,
        .ident = {
            .name = strdup(name),
        }
    };
    return memdup(&x, sizeof(expr_t));
}

typedef struct {
    scope_t *topScope;
} walker_t;

static expr_t *find_type(walker_t *w, expr_t *lhs, expr_t *expr) {
    assert(expr);
    switch (expr->type) {
    case ast_EXPR_BASIC_LIT:
        print("finding basic type");
        return find_basic_type(expr->basic_lit.kind);
    case ast_EXPR_IDENT:
        print("finding type of %s", expr->ident.name);
        if (!expr->ident.obj) {
            panic("find_type: not resolved: %s", expr->ident.name);
        }
        switch (expr->ident.obj->kind) {
        case obj_kind_VALUE:
            return expr->ident.obj->decl->value.type;
        default:
            panic("find_type: unknown kind: %d", obj_kind_VALUE);
        }
    case ast_EXPR_COMPOUND:
        if (lhs == NULL) {
            panic("find_type: unable to resolve composite expr");
        }
        return lhs;
    default:
        panic("find_type: unknown expr: %d", expr->type);
        break;
    }
    return NULL;
}

static bool match_types(expr_t *a, expr_t *b) {
    if (a->type == ast_TYPE_QUAL) {
        a = a->qual.type;
    }
    if (b->type == ast_TYPE_QUAL) {
        b = b->qual.type;
    }
    if (a->type == ast_EXPR_IDENT && b->type == ast_EXPR_IDENT) {
        return streq(a->ident.name, b->ident.name);
    }
    print("a type: %d", a->type);
    print("b type: %d", b->type);
    return false;
}

static expr_t *type_from_ident(expr_t *ident) {
    print("type_from_ident: %s", ident->ident.name);
    assert(ident->ident.obj);
    decl_t *decl = ident->ident.obj->decl;
    switch (decl->type) {
    default:
        panic("type_from_ident: bad decl: %d", decl->type);
    }
    return NULL;
}

static void walk_expr(walker_t *w, expr_t *expr) {
    switch (expr->type) {
    case ast_EXPR_BASIC_LIT:
    case ast_EXPR_COMPOUND:
        break;
    case ast_EXPR_IDENT:
        scope_resolve(w->topScope, expr);
        break;
    case ast_EXPR_SELECTOR:
        {
            walk_expr(w, expr->selector.x);
            /* walk_expr(w, expr->selector.sel); */
            expr_t *type = find_type(w, NULL, expr->selector.x);
            if (type->type == ast_TYPE_PTR) {
                type = type->ptr.type;
            } else {
                assert(expr->selector.tok != token_ARROW);
            }
            for (;;) {
                if (type->type == ast_EXPR_IDENT) {
                    type = type_from_ident(type);
                } else if (type->type == ast_TYPE_QUAL) {
                    type = type->qual.type;
                } else {
                    break;
                }
            }
            print("type %d", type->type);
            assert(type->type == ast_TYPE_STRUCT);
            /* expr_t *type = find_type(w, NULL, expr->selector.sel); */
            /* walk_expr(w, expr->selector.sel); */
        }
        panic("walk_expr: unknown expr: selector");
        break;
    case ast_TYPE_ENUM:
        for (int i = 0; expr->enum_.enums[i]; i++) {
            scope_declare(w->topScope, expr->enum_.enums[i]);
        }
        break;
    case ast_TYPE_PTR:
        walk_expr(w, expr->ptr.type);
        break;
    case ast_TYPE_QUAL:
        walk_expr(w, expr->qual.type);
        break;
    case ast_TYPE_STRUCT:
        w->topScope = scope_new(w->topScope);
        for (int i = 0; expr->struct_.fields[i]; i++) {
            scope_declare(w->topScope, expr->struct_.fields[i]);
        }
        w->topScope = w->topScope->outer;
        break;
    default:
        panic("walk_expr: unknown expr: %d", expr->type);
        break;
    }
}

static void walk_stmt(walker_t *w, stmt_t *stmt) {
    switch (stmt->type) {
    case ast_STMT_BLOCK:
        for (int i = 0; stmt->block.stmts[i]; i++) {
            walk_stmt(w, stmt->block.stmts[i]);
        }
        break;
    case ast_STMT_EXPR:
        walk_expr(w, stmt->expr.x);
        break;
    case ast_STMT_IF:
        walk_expr(w, stmt->if_.cond);
        walk_stmt(w, stmt->if_.body);
        break;
    case ast_STMT_RETURN:
        // TODO: assert that expr type equals return type
        break;
    default:
        panic("walk_stmt: unknown stmt: %d", stmt->type);
    }
}

static void walk_decl(walker_t *w, decl_t *decl) {
    switch (decl->type) {
    case ast_DECL_FUNC:
        break;
    case ast_DECL_IMPORT:
        break;
    case ast_DECL_TYPEDEF:
        print("walk_decl: walking typedef %s", decl->typedef_.name->ident.name);
        walk_expr(w, decl->typedef_.type);
        scope_declare(w->topScope, decl);
        break;
    case ast_DECL_VALUE:
        if (decl->value.value != NULL) {
            walk_expr(w, decl->value.value);
        }
        if (decl->value.type == NULL) {
            decl->value.type = find_type(w, decl->value.type, decl->value.value);
        } else if (decl->value.value != NULL) {
            assert(match_types(decl->value.type,
                        find_type(w, decl->value.type, decl->value.value)));
        }
        scope_declare(w->topScope, decl);
        break;
    default:
        panic("walk_decl: not implemented: %d", decl->type);
    }
}

static void walk_func(walker_t *w, decl_t *decl) {
    if (decl->type == ast_DECL_FUNC && decl->func.body) {
        print("walk_func: walking func %s", decl->func.name->ident.name);
        w->topScope = scope_new(w->topScope);
        for (int i = 0; decl->func.type->func.params[i]; i++) {
            decl_t *param = decl->func.type->func.params[i];
            if (param->type) {
                print("declaring param `%s`", param->field.name->ident.name);
                scope_declare(w->topScope, param);
                print("walking type");
                walk_expr(w, param->field.type);
            }
        }
        walk_stmt(w, decl->func.body);
        w->topScope = w->topScope->outer;
    }
}

extern void walk_file(file_t *file) {
    map_iter_t iter = map_iter(&file->scope->objects);
    char *key = NULL;
    void *val = NULL;
    while (map_iter_next(&iter, &key, val)) {
        print("iter: %s", key);
    }
    walker_t w = {.topScope = scope_new(NULL)};
    for (int i = 0; file->decls[i] != NULL; i++) {
        walk_decl(&w, file->decls[i]);
    }
    for (int i = 0; file->decls[i] != NULL; i++) {
        walk_func(&w, file->decls[i]);
    }
}
