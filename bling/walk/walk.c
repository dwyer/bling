#include "bling/ast/ast.h"
#include "bling/emitter/emit.h"

static void printlg(const char *fmt, ...) {}

#define printlg(...) print(__VA_ARGS__)

static char *native_types[] = {
    // native types
    "char",
    "float",
    "int",
    "void",

    // libc types
    "DIR",
    "FILE",
    "bool",
    "size_t",
    "uint32_t",
    "uint64_t",
    "uintptr_t",
    "va_list",

    NULL,
};

extern void declare_builtins(scope_t *s) {
    for (int i = 0; native_types[i] != NULL; i++) {
        expr_t name = {
            .type = ast_EXPR_IDENT,
            .ident = {
                .name = strdup(native_types[i]),
            },
        };
        decl_t decl = {
            .type = ast_DECL_NATIVE,
            .native = {
                .name = esc(name),
            },
        };
        scope_declare(s, esc(decl));
    }
}

typedef struct {
    scope_t *topScope;
    expr_t *result;
} walker_t;

static void walker_openScope(walker_t *w) {
    printlg("opening scope");
    w->topScope = scope_new(w->topScope);
}

static void walker_closeScope(walker_t *w) {
    printlg("closing scope");
    w->topScope = w->topScope->outer;
}

extern void printScope(scope_t *s) {
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

static void type_check(expr_t *a, expr_t *b) {
    if (!match_types(a, b)) {
        emitter_t e = {};
        emit_string(&e, "mismatched types: ");
        emit_string(&e, "`");
        print_type(&e, a);
        emit_string(&e, "` and `");
        print_type(&e, b);
        emit_string(&e, "`");
        panic(strings_Builder_string(&e.builder));
    }
}

static expr_t *lookup_typedef(expr_t *ident) {
    assert(ident->ident.obj);
    decl_t *decl = ident->ident.obj->decl;
    assert(decl);
    assert(decl->type == ast_DECL_TYPEDEF);
    expr_t *type = decl->typedef_.type;
    return type;
}

static expr_t *unwind_typedef(expr_t *type) {
    for (;;) {
        assert(type);
        switch (type->type) {
        case ast_EXPR_IDENT:
            type = lookup_typedef(type);
            break;
        case ast_TYPE_QUAL:
            type = type->qual.type;
            break;
        default:
            assert(type);
            return type;
        }
    }
}

static void walk_type(walker_t *w, expr_t *expr) {
    switch (expr->type) {

    case ast_EXPR_IDENT:
        scope_resolve(w->topScope, expr);
        break;

    case ast_TYPE_ENUM:
        for (int i = 0; expr->enum_.enums[i]; i++) {
            scope_declare(w->topScope, expr->enum_.enums[i]);
        }
        break;

    case ast_TYPE_PTR:
        walk_type(w, expr->ptr.type);
        break;

    case ast_TYPE_QUAL:
        walk_type(w, expr->qual.type);
        break;

    case ast_TYPE_STRUCT:
        walker_openScope(w);
        for (int i = 0; expr->struct_.fields[i]; i++) {
            scope_declare(w->topScope, expr->struct_.fields[i]);
        }
        walker_closeScope(w);
        break;

    default:
        panic("walk_type: unknown expr: %d", expr->type);
        break;
    }
}

static expr_t *walk_expr(walker_t *w, expr_t *expr) {
    switch (expr->type) {
    case ast_EXPR_BINARY:
        {
            expr_t *typ1 = walk_expr(w, expr->binary.x);
            expr_t *typ2 = walk_expr(w, expr->binary.y);
            type_check(typ1, typ2);
            return typ1;
        }

    case ast_EXPR_BASIC_LIT:
        {
            const char *name = NULL;
            token_t kind = expr->basic_lit.kind;
            switch (kind) {
            case token_INT:
                name = "int";
                break;
            case token_FLOAT:
                name = "float";
                break;
            default:
                panic("walk_expr: not implmented: %s", token_string(kind));
            }
            expr_t x = {
                .type = ast_EXPR_IDENT,
                .ident = {
                    .name = strdup(name),
                }
            };
            return esc(x);
        }

    case ast_EXPR_COMPOUND:
        return NULL;

    case ast_EXPR_CALL:
        {
            expr_t *type = walk_expr(w, expr->call.func);
            switch (type->type) {
            case ast_TYPE_PTR:
                type = type->ptr.type;
                assert(type->type == ast_TYPE_FUNC); // TODO handle builtins
            case ast_TYPE_FUNC:
                for (int i = 0; expr->call.args[i]; i++) {
                    assert(type->func.params[i]);
                    decl_t *param = type->func.params[i];
                    assert(param->type == ast_DECL_FIELD);
                    expr_t *type = walk_expr(w, expr->call.args[i]);
                    type_check(param->field.type, type);
                }
                return type->func.result;
            default:
                panic("call expr is not a func");
            }
            return NULL;
        }

    case ast_EXPR_CAST:
        {
            walk_type(w, expr->cast.type);
            walk_expr(w, expr->cast.expr);
            // TODO: apply type to compound lit
            return expr->cast.type;
        }

    case ast_EXPR_IDENT:
        {
            scope_resolve(w->topScope, expr);
            object_t *obj = expr->ident.obj;
            decl_t *decl = obj->decl;
            switch (obj->kind) {
            case obj_kind_FUNC:
                return decl->func.type;
            case obj_kind_TYPE:
                return decl->typedef_.type;
            case obj_kind_VALUE:
                return decl->value.type;
            }
        }

    case ast_EXPR_PAREN:
        return walk_expr(w, expr->paren.x);

    case ast_EXPR_SELECTOR:
        {
            expr_t *type = walk_expr(w, expr->selector.x);
            if (type->type == ast_TYPE_PTR) {
                expr->selector.tok = token_ARROW;
                type = type->ptr.type;
            } else {
                assert(expr->selector.tok != token_ARROW);
            }
            type = unwind_typedef(type);
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
            if (!resolved) {
                panic("struct has no field `%s`", expr->selector.sel->ident.name);
            }
            return type;
        }

    case ast_EXPR_UNARY:
        return walk_expr(w, expr->unary.x);

    default:
        panic("walk_expr: unknown expr: %d", expr->type);
    }
    return NULL;
}

static void walk_decl(walker_t *w, decl_t *decl);

static void walk_stmt(walker_t *w, stmt_t *stmt) {
    switch (stmt->type) {
    case ast_STMT_ASSIGN:
        {
            switch (stmt->assign.x->type) {
            case ast_EXPR_IDENT:
            case ast_EXPR_SELECTOR:
                break;
            default:
                panic("walk_stmt: lhs of assign stmt must be ident or selector");
            }
            expr_t *a = walk_expr(w, stmt->assign.x);
            expr_t *b = walk_expr(w, stmt->assign.y);
            (void)a;
            (void)b;
        }
        break;
    case ast_STMT_BLOCK:
        walker_openScope(w);
        for (int i = 0; stmt->block.stmts[i]; i++) {
            walk_stmt(w, stmt->block.stmts[i]);
        }
        walker_closeScope(w);
        break;
    case ast_STMT_DECL:
        walk_decl(w, stmt->decl);
        break;
    case ast_STMT_EXPR:
        walk_expr(w, stmt->expr.x);
        break;
    case ast_STMT_EMPTY:
        break;
    case ast_STMT_IF:
        walk_expr(w, stmt->if_.cond);
        walk_stmt(w, stmt->if_.body);
        break;

    case ast_STMT_ITER:
        if (stmt->iter.init || stmt->iter.post) {
            walker_openScope(w);
        }
        if (stmt->iter.init) {
            walk_stmt(w, stmt->iter.init);
        }
        if (stmt->iter.cond) {
            walk_expr(w, stmt->iter.cond);
        }
        if (stmt->iter.post) {
            walk_stmt(w, stmt->iter.post);
        }
        walk_stmt(w, stmt->iter.body);
        if (stmt->iter.init || stmt->iter.post) {
            walker_closeScope(w);
        }
        break;

    case ast_STMT_JUMP:
        /* TODO walk in label scope */
        break;

    case ast_STMT_RETURN:
        if (stmt->return_.x) {
            expr_t *type = walk_expr(w, stmt->return_.x);
            assert(w->result);
            type_check(w->result, type);
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
        walk_type(w, decl->typedef_.type);
        scope_declare(w->topScope, decl);
        break;
    case ast_DECL_VALUE:
        {
            expr_t *val_type = NULL;
            if (decl->value.type != NULL) {
                walk_type(w, decl->value.type);
            }
            if (decl->value.value != NULL) {
                val_type = walk_expr(w, decl->value.value);
            }
            if (decl->value.type == NULL) {
                decl->value.type = val_type;
            }
            if (val_type != NULL) {
                type_check(decl->value.type, val_type);
            }
            scope_declare(w->topScope, decl);
            break;
        }
    default:
        panic("walk_decl: not implemented: %d", decl->type);
    }
}

static void walk_func(walker_t *w, decl_t *decl) {
    assert(decl);
    if (decl->type == ast_DECL_FUNC && decl->func.body) {
        printlg("walk_func: walking func %s", decl->func.name->ident.name);
        expr_t *type = decl->func.type;
        walker_openScope(w);
        for (int i = 0; type->func.params && type->func.params[i]; i++) {
            decl_t *param = type->func.params[i];
            if (param->field.type) {
                printlg("walk_func: walking type of param `%s`", param->field.name->ident.name);
                walk_type(w, param->field.type);
                printlg("walk_func: declaring param `%s`", param->field.name->ident.name);
                scope_declare(w->topScope, param);
            }
        }
        w->result = decl->func.type->func.result;
        walk_stmt(w, decl->func.body);
        w->result = NULL;
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
