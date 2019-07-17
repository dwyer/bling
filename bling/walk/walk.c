#include "bling/ast/ast.h"

static void printlg(const char *fmt, ...) {}

#define printlg(...) print(__VA_ARGS__)

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

static void walker_openScope(walker_t *w) {
    printlg("opening scope");
    w->topScope = scope_new(w->topScope);
}

static void walker_closeScope(walker_t *w) {
    printlg("closing scope");
    w->topScope = w->topScope->outer;
}

static expr_t *find_type(walker_t *w, expr_t *lhs, expr_t *expr) {
    assert(expr);
    switch (expr->type) {
    case ast_EXPR_BASIC_LIT:
        printlg("finding basic type");
        return find_basic_type(expr->basic_lit.kind);
    case ast_EXPR_IDENT:
        printlg("finding type of `%s`", expr->ident.name);
        if (!expr->ident.obj) {
            panic("find_type: not resolved: `%s`", expr->ident.name);
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

static void printScope(scope_t *s) {
    int indent = 1;
    while (s) {
        map_iter_t iter = map_iter(&s->objects);
        char *key = NULL;
        while (map_iter_next(&iter, &key, NULL)) {
            for (int i = 0; i < indent; i++) {
                fputs("\t", stderr);
            }
            printlg("%s", key);
        }
        s = s->outer;
        indent++;
    }
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
    printlg("a type: %d", a->type);
    printlg("b type: %d", b->type);
    return false;
}

static expr_t *type_from_ident(expr_t *ident) {
    printlg("type_from_ident: %s", ident->ident.name);
    assert(ident->ident.obj);
    decl_t *decl = ident->ident.obj->decl;
    assert(decl);
    assert(decl->type == ast_DECL_TYPEDEF);
    expr_t *type = decl->typedef_.type;
    return type;
}

static expr_t *walk_expr(walker_t *w, expr_t *expr) {
    switch (expr->type) {
    case ast_EXPR_UNARY:
        return walk_expr(w, expr->unary.x);
    case ast_EXPR_CAST:
        {
            expr_t *a = walk_expr(w, expr->cast.expr);
            expr_t *b = walk_expr(w, expr->cast.type);
            return b;
        }
    case ast_EXPR_BINARY:
        {
            expr_t *a = walk_expr(w, expr->binary.x);
            expr_t *b = walk_expr(w, expr->binary.y);
            return b;
        }
    case ast_EXPR_BASIC_LIT:
    case ast_EXPR_COMPOUND:
        return NULL;
    case ast_EXPR_CALL:
        {
            expr_t *type = walk_expr(w, expr->call.func);
            if (type->type == ast_TYPE_PTR) {
                type = type->ptr.type;
            }
            assert(type->type == ast_TYPE_FUNC);
            return type->func.result;
        }
    case ast_EXPR_IDENT:
        printlg("walk_expr: resolving `%s`", expr->ident.name);
        scope_resolve(w->topScope, expr);
        return NULL;
    case ast_EXPR_SELECTOR:
        {
            expr_t *a = walk_expr(w, expr->selector.x);
            expr_t *type = find_type(w, NULL, expr->selector.x);
            if (type->type == ast_TYPE_PTR) {
                type = type->ptr.type;
            } else {
                assert(expr->selector.tok != token_ARROW);
            }
            for (;;) {
                if (!type) {
                    printlg("no type");
                    break;
                }
                switch (type->type) {
                case ast_EXPR_IDENT:
                    printlg("walk_expr: getting type from ident");
                    type = type_from_ident(type);
                    continue;
                case ast_TYPE_QUAL:
                    printlg("walk_expr: getting type from qual");
                    type = type->qual.type;
                    continue;
                default:
                    goto out;
                }
            }
out:
            assert(type);
            assert(type->type == ast_TYPE_STRUCT);
            printlg("selector: %s", expr->selector.sel->ident.name);
            bool resolved = false;
            for (int i = 0; type->struct_.fields[i]; i++) {
                decl_t *field = type->struct_.fields[i];
                printlg("field: %s", field->field.name->ident.name);
                if (streq(expr->selector.sel->ident.name,
                            field->field.name->ident.name)) {
                    resolved = true;
                    type = field->field.type;
                    break;
                }
            }
            assert(resolved);
            return type;
        }
    case ast_TYPE_ENUM:
        for (int i = 0; expr->enum_.enums[i]; i++) {
            scope_declare(w->topScope, expr->enum_.enums[i]);
        }
        return expr;
    case ast_TYPE_PTR:
        walk_expr(w, expr->ptr.type);
        return expr;
    case ast_TYPE_QUAL:
        walk_expr(w, expr->qual.type);
        return expr;
    case ast_TYPE_STRUCT:
        walker_openScope(w);
        for (int i = 0; expr->struct_.fields[i]; i++) {
            scope_declare(w->topScope, expr->struct_.fields[i]);
        }
        walker_closeScope(w);
        return expr;
    default:
        panic("walk_expr: unknown expr: %d", expr->type);
    }
    return NULL;
}

static void walk_decl(walker_t *w, decl_t *decl);

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
    case ast_STMT_DECL:
        walk_decl(w, stmt->decl);
        break;
    case ast_STMT_IF:
        walk_expr(w, stmt->if_.cond);
        walk_stmt(w, stmt->if_.body);
        break;
    case ast_STMT_RETURN:
        if (stmt->return_.x) {
            expr_t *type = walk_expr(w, stmt->return_.x);
            // TODO: assert that expr type equals return type
        }
        break;
    default:
        panic("walk_stmt: unknown stmt: %d", stmt->type);
    }
}

static void walk_decl(walker_t *w, decl_t *decl) {
    switch (decl->type) {
    case ast_DECL_FUNC:
        scope_declare(w->topScope, decl);
        break;
    case ast_DECL_IMPORT:
        break;
    case ast_DECL_TYPEDEF:
        printlg("walk_decl: walking typedef %s", decl->typedef_.name->ident.name);
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
    assert(decl);
    if (decl->type == ast_DECL_FUNC && decl->func.body) {
        printlg("walk_func: walking func %s", decl->func.name->ident.name);
        walker_openScope(w);
        for (int i = 0; decl->func.type->func.params && decl->func.type->func.params[i]; i++) {
            decl_t *param = decl->func.type->func.params[i];
            printlg("got param");
            if (param->type) {
                printlg("walk_func: walking type of param `%s`", param->field.name->ident.name);
                printScope(w->topScope);
                walk_expr(w, param->field.type);
                printlg("walk_func: declaring param `%s`", param->field.name->ident.name);
                scope_declare(w->topScope, param);
            }
        }
        walk_stmt(w, decl->func.body);
        walker_closeScope(w);
    }
}

extern void walk_file(file_t *file) {
    walker_t w = {.topScope = scope_new(file->scope)};
    for (int i = 0; file->decls[i] != NULL; i++) {
        walk_decl(&w, file->decls[i]);
    }
    for (int i = 0; file->decls[i] != NULL; i++) {
        walk_func(&w, file->decls[i]);
    }
}
