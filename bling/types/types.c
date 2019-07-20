#include "bling/ast/ast.h"
#include "bling/emitter/emit.h"

static void printlg(const char *fmt, ...) {}

#define printlg(...) print(__VA_ARGS__)

static char *builtins[] = {
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
    // builtin funcs
    "esc",
    // sentinel
    NULL,
};

extern void declare_builtins(scope_t *s) {
    for (int i = 0; builtins[i] != NULL; i++) {
        expr_t name = {
            .type = ast_EXPR_IDENT,
            .ident = {
                .name = strdup(builtins[i]),
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
} checker_t;

static void checker_openScope(checker_t *w) {
    printlg("opening scope");
    w->topScope = scope_new(w->topScope);
}

static void checker_closeScope(checker_t *w) {
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

extern bool types_isIdent(expr_t *expr) {
    return expr->type == ast_EXPR_IDENT;
}

extern bool types_isVoid(expr_t *type) {
    return types_isIdent(type) && streq(type->ident.name, "void");
}

extern bool types_isVoidPtr(expr_t *type) {
    if (type->type == ast_TYPE_PTR) {
        return types_isVoid(type->ptr.type);
    }
    return false;
}

static bool types_match(expr_t *a, expr_t *b) {
    if (a->type == ast_TYPE_QUAL) {
        if (b->type == ast_TYPE_QUAL) {
            if (a->qual.qual != b->qual.qual) {
                return false;
            }
            b = b->qual.type;
        }
        a = a->qual.type;
    }
    // TODO match ptrs with arrays
    if (a->type != b->type) {
        return false;
    }
    switch (a->type) {
    case ast_EXPR_IDENT:
        return streq(a->ident.name, b->ident.name);
    case ast_TYPE_PTR:
        if (types_isVoidPtr(a) || types_isVoidPtr(b)) {
            return true;
        }
        return types_match(a->ptr.type, b->ptr.type);
    case ast_TYPE_QUAL:
        return false;
    default:
        return false;
    }
}

static void types_check(expr_t *a, expr_t *b) {
    if (!types_match(a, b)) {
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

static void check_type(checker_t *w, expr_t *expr) {
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
        check_type(w, expr->ptr.type);
        break;

    case ast_TYPE_QUAL:
        check_type(w, expr->qual.type);
        break;

    case ast_TYPE_STRUCT:
        checker_openScope(w);
        for (int i = 0; expr->struct_.fields[i]; i++) {
            scope_declare(w->topScope, expr->struct_.fields[i]);
        }
        checker_closeScope(w);
        break;

    default:
        panic("check_type: unknown expr: %d", expr->type);
        break;
    }
}

static expr_t *check_expr(checker_t *w, expr_t *expr) {
    switch (expr->type) {
    case ast_EXPR_BINARY:
        {
            expr_t *typ1 = check_expr(w, expr->binary.x);
            expr_t *typ2 = check_expr(w, expr->binary.y);
            types_check(typ1, typ2);
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
                panic("check_expr: not implmented: %s", token_string(kind));
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
            expr_t *func = expr->call.func;
            expr_t *type = check_expr(w, func);
            if (func->type == ast_EXPR_IDENT && streq(func->ident.name, "esc")) {
                assert(expr->call.args[0] && !expr->call.args[1]);
                expr_t *type = check_expr(w, expr->call.args[0]);
                expr_t x = {
                    .type = ast_TYPE_PTR,
                    .ptr = {
                        .type = type,
                    }
                };
                return esc(x);
            } else {
                if (type->type == ast_TYPE_PTR) {
                    type = type->ptr.type;
                }
                assert(type->type == ast_TYPE_FUNC); // TODO handle builtins
                for (int i = 0; expr->call.args[i]; i++) {
                    assert(type->func.params[i]);
                    decl_t *param = type->func.params[i];
                    assert(param->type == ast_DECL_FIELD);
                    expr_t *type = check_expr(w, expr->call.args[i]);
                    types_check(param->field.type, type);
                }
                return type->func.result;
            }
        }

    case ast_EXPR_CAST:
        {
            check_type(w, expr->cast.type);
            check_expr(w, expr->cast.expr);
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
        return check_expr(w, expr->paren.x);

    case ast_EXPR_SELECTOR:
        {
            expr_t *type = check_expr(w, expr->selector.x);
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
        return check_expr(w, expr->unary.x);

    default:
        panic("check_expr: unknown expr: %d", expr->type);
    }
    return NULL;
}

static void check_decl(checker_t *w, decl_t *decl);

static void check_stmt(checker_t *w, stmt_t *stmt) {
    switch (stmt->type) {
    case ast_STMT_ASSIGN:
        {
            expr_t *lhs = stmt->assign.x;
            if (lhs->type == ast_EXPR_UNARY && lhs->unary.op == token_MUL) {
                lhs = lhs->unary.x;
            }
            switch (lhs->type) {
            case ast_EXPR_IDENT:
            case ast_EXPR_SELECTOR:
                break;
            default:
                panic("check_stmt: lhs of assign stmt must be ident or selector");
            }
            expr_t *a = check_expr(w, stmt->assign.x);
            expr_t *b = check_expr(w, stmt->assign.y);
            (void)a;
            (void)b;
        }
        break;
    case ast_STMT_BLOCK:
        checker_openScope(w);
        for (int i = 0; stmt->block.stmts[i]; i++) {
            check_stmt(w, stmt->block.stmts[i]);
        }
        checker_closeScope(w);
        break;
    case ast_STMT_DECL:
        check_decl(w, stmt->decl);
        break;
    case ast_STMT_EXPR:
        check_expr(w, stmt->expr.x);
        break;
    case ast_STMT_EMPTY:
        break;
    case ast_STMT_IF:
        check_expr(w, stmt->if_.cond);
        check_stmt(w, stmt->if_.body);
        break;

    case ast_STMT_ITER:
        if (stmt->iter.init || stmt->iter.post) {
            checker_openScope(w);
        }
        if (stmt->iter.init) {
            check_stmt(w, stmt->iter.init);
        }
        if (stmt->iter.cond) {
            check_expr(w, stmt->iter.cond);
        }
        if (stmt->iter.post) {
            check_stmt(w, stmt->iter.post);
        }
        check_stmt(w, stmt->iter.body);
        if (stmt->iter.init || stmt->iter.post) {
            checker_closeScope(w);
        }
        break;

    case ast_STMT_JUMP:
        /* TODO walk in label scope */
        break;

    case ast_STMT_RETURN:
        if (stmt->return_.x) {
            expr_t *type = check_expr(w, stmt->return_.x);
            assert(w->result);
            types_check(w->result, type);
        }
        break;
    default:
        panic("check_stmt: unknown stmt: %d", stmt->type);
    }
}

static void check_decl(checker_t *w, decl_t *decl) {
    switch (decl->type) {
    case ast_DECL_FUNC:
        scope_declare(w->topScope, decl);
        break;
    case ast_DECL_IMPORT:
        break;
    case ast_DECL_TYPEDEF:
        printlg("check_decl: walking typedef %s", decl->typedef_.name->ident.name);
        check_type(w, decl->typedef_.type);
        scope_declare(w->topScope, decl);
        break;
    case ast_DECL_VALUE:
        {
            expr_t *val_type = NULL;
            if (decl->value.type != NULL) {
                check_type(w, decl->value.type);
            }
            if (decl->value.value != NULL) {
                val_type = check_expr(w, decl->value.value);
            }
            if (decl->value.type == NULL) {
                decl->value.type = val_type;
            }
            if (val_type != NULL) {
                types_check(decl->value.type, val_type);
            }
            scope_declare(w->topScope, decl);
            break;
        }
    default:
        panic("check_decl: not implemented: %d", decl->type);
    }
}

static void check_func(checker_t *w, decl_t *decl) {
    assert(decl);
    if (decl->type == ast_DECL_FUNC && decl->func.body) {
        printlg("check_func: walking func %s", decl->func.name->ident.name);
        expr_t *type = decl->func.type;
        checker_openScope(w);
        for (int i = 0; type->func.params && type->func.params[i]; i++) {
            printlg("getting param");
            decl_t *param = type->func.params[i];
            printlg("got param");
            assert(param->type == ast_DECL_FIELD);
            if (param->field.name) {
                printlg("check_func: walking type of param `%s`", param->field.name->ident.name);
                check_type(w, param->field.type);
                printlg("check_func: declaring param `%s`", param->field.name->ident.name);
                scope_declare(w->topScope, param);
            }
        }
        w->result = decl->func.type->func.result;
        check_stmt(w, decl->func.body);
        w->result = NULL;
        checker_closeScope(w);
    }
}

extern void types_checkFile(file_t *file) {
    checker_t w = {.topScope = scope_new(file->scope)};
    for (int i = 0; file->decls[i] != NULL; i++) {
        check_decl(&w, file->decls[i]);
    }
    for (int i = 0; file->decls[i] != NULL; i++) {
        check_func(&w, file->decls[i]);
    }
}
