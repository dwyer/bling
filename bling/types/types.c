#include "bling/ast/ast.h"
#include "bling/emitter/emitter.h"

extern void printlg(const char *fmt, ...) {}

#define printlg(...) print(__VA_ARGS__)

static void scope_declare(scope_t *s, decl_t *decl) {
    obj_kind_t kind;
    expr_t *ident = NULL;
    switch (decl->type) {
    case ast_DECL_FIELD:
        kind = obj_kind_VALUE;
        ident = decl->field.name;
        break;
    case ast_DECL_FUNC:
        kind = obj_kind_FUNC;
        ident = decl->func.name;
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
        panic("already declared: %s", ident->ident.name);
    }
    object_t *obj = object_new(kind, ident->ident.name);
    obj->decl = decl;
    ident->ident.obj = obj;
    object_t *alt = scope_insert(s, obj);
    if (alt != NULL) {
        assert(alt->kind == kind);
        bool redecl = false;
        switch (kind) {
        case obj_kind_FUNC:
            // TODO compare types
            redecl = alt->decl->func.body == NULL;
            break;
        case obj_kind_TYPE:
            if (alt->decl->typedef_.type->type == ast_TYPE_STRUCT) {
                redecl = alt->decl->typedef_.type->struct_.fields == NULL;
            }
            break;
        case obj_kind_VALUE:
            // TODO compare types
            redecl = alt->decl->value.value == NULL;
            break;
        default:
            print("unknown kind: %d", kind);
            break;
        }
        if (!redecl) {
            panic("already declared: %s", ident->ident.name);
        }
        print("redeclaring %s", obj->name);
        alt->decl = decl;
    }
}

static struct {
    char *name;
    int size;
    bool arith;
} natives[] = {
    // native types
    {"char", sizeof(char), true},
    {"float", sizeof(float), true},
    {"int", sizeof(int), true},
    {"void", sizeof(void)},
    // libc types
    {"DIR", sizeof(DIR)},
    {"FILE", sizeof(FILE)},
    {"size_t", sizeof(size_t), true},
    {"uint32_t", sizeof(uint32_t), true},
    {"uint64_t", sizeof(uint64_t), true},
    {"uintptr_t", sizeof(uintptr_t), true},
    {"va_list", sizeof(va_list)},
    // sentinel
    {NULL},
};

static void declare_natives(scope_t *s) {
    for (int i = 0; natives[i].name != NULL; i++) {
        expr_t name = {
            .type = ast_EXPR_IDENT,
            .ident = {
                .name = strdup(natives[i].name),
            },
        };
        expr_t type = {
            .type = ast_TYPE_NATIVE,
            .native = {
                .name = strdup(natives[i].name),
                .size = natives[i].size,
            },
        };
        decl_t decl = {
            .type = ast_DECL_TYPEDEF,
            .typedef_ = {
                .name = esc(name),
                .type = esc(type),
            },
        };
        printlg("declare_natives: declaring %s", decl.typedef_.name->ident.name);
        scope_declare(s, esc(decl));
    }
}

extern void declare_builtins(scope_t *s) {
    declare_natives(s);
}

typedef struct {
    scope_t *topScope;
    expr_t *result;
    bool strict;
} checker_t;

static void checker_openScope(checker_t *w) {
    w->topScope = scope_new(w->topScope);
}

static void checker_closeScope(checker_t *w) {
    scope_t *inner = w->topScope;
    w->topScope = w->topScope->outer;
    scope_free(inner);
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

extern char *types_declString(decl_t *decl) {
    emitter_t e = {};
    print_decl(&e, decl);
    return strings_Builder_string(&e.builder);
}

extern char *types_exprString(expr_t *expr) {
    emitter_t e = {};
    print_expr(&e, expr);
    return strings_Builder_string(&e.builder);
}

extern char *types_stmtString(stmt_t *stmt) {
    emitter_t e = {};
    print_stmt(&e, stmt);
    return strings_Builder_string(&e.builder);
}

extern char *types_typeString(expr_t *expr) {
    emitter_t e = {};
    print_type(&e, expr);
    return strings_Builder_string(&e.builder);
}

extern bool types_isIdent(expr_t *expr) {
    return expr->type == ast_EXPR_IDENT;
}

extern bool types_isVoid(expr_t *type) {
    if (type->type == ast_TYPE_QUAL) {
        type = type->qual.type;
    }
    return types_isIdent(type) && streq(type->ident.name, "void");
}

extern bool types_isVoidPtr(expr_t *type) {
    if (type->type == ast_EXPR_STAR) {
        return types_isVoid(type->star.x);
    }
    return false;
}

static expr_t *types_makeIdent(const char *name) {
    expr_t x = {
        .type = ast_EXPR_IDENT,
        .ident = {
            .name = strdup(name),
        },
    };
    return esc(x);
}

static expr_t *types_makePtr(expr_t *type) {
    expr_t x = {
        .type = ast_EXPR_STAR,
        .star = {
            .x = type,
        }
    };
    return esc(x);
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
        switch (type->type) {
        case ast_TYPE_NATIVE:
        case ast_TYPE_STRUCT:
            return type;
        case ast_EXPR_IDENT:
            type = lookup_typedef(type);
            break;
        case ast_TYPE_QUAL:
            type = type->qual.type;
            break;
        default:
            panic("unwind_typedef: not impl: %s", types_typeString(type));
        }
    }
}

static bool types_areIdentical(expr_t *a, expr_t *b) {
    if (a == b) {
        return true;
    }
    assert(a);
    assert(b);
    if (a->type != b->type) {
        return false;
    }
    switch (a->type) {
    case ast_EXPR_IDENT:
        return b->type == ast_EXPR_IDENT && a->ident.obj == a->ident.obj;
    case ast_EXPR_STAR:
        if (types_isVoidPtr(a) || types_isVoidPtr(b)) {
            return true;
        }
        return types_areIdentical(a->star.x, b->star.x);
    case ast_TYPE_QUAL:
        return types_areIdentical(a->qual.type, b->qual.type);
    default:
        panic("unreachable: %s == %s", types_typeString(a), types_typeString(b));
        return false;
    }
}

static bool types_areAssignable(expr_t *a, expr_t *b) {
    if (types_areIdentical(a, b)) {
        return true;
    }
    if (a->type == ast_TYPE_QUAL) {
        if (b->type == ast_TYPE_QUAL) {
            if (a->qual.qual != b->qual.qual) {
                return false;
            }
            b = b->qual.type;
        }
        a = a->qual.type;
    }
    if (types_isVoidPtr(a) || types_isVoidPtr(b)) {
        return true;
    }
    if (a->type == ast_EXPR_STAR && b->type == ast_EXPR_STAR) {
        return types_areAssignable(a->star.x, b->star.x);
    }
    if (a->type == ast_EXPR_STAR && b->type == ast_TYPE_ARRAY) {
        return types_areAssignable(a->star.x, b->array.elt);
    }
    if (a->type == ast_TYPE_ARRAY && b->type == ast_EXPR_STAR) {
        return types_areAssignable(a->array.elt, b->star.x);
    }
    while (a->type == ast_EXPR_IDENT) {
        a = lookup_typedef(a);
    }
    while (b->type == ast_EXPR_IDENT) {
        b = lookup_typedef(b);
    }
    if (b->type == ast_TYPE_ENUM && a->type == ast_TYPE_NATIVE && streq(a->native.name, "int")) {
        return true;
    }
    if (a->type == ast_TYPE_ENUM && b->type == ast_TYPE_NATIVE && streq(b->native.name, "int")) {
        return true;
    }
    return types_areIdentical(a, b);
}

static bool types_isNative(expr_t *type, const char *name) {
    switch (type->type) {
    case ast_EXPR_IDENT:
        return streq(type->ident.name, name);
    case ast_TYPE_NATIVE:
        return true;
    default:
        return false;
    }
}

static bool types_areComparable(expr_t *a, expr_t *b) {
    if (types_areAssignable(a, b)) {
        return true;
    }
    if (a->type == ast_TYPE_QUAL) {
        return types_areComparable(a->qual.type, b);
    }
    if (b->type == ast_TYPE_QUAL) {
        return types_areComparable(a, b->qual.type);
    }
    if (a->type == ast_EXPR_STAR && types_isNative(b, "int")) {
        return true;
    }
    return types_areAssignable(a, b);
}

static expr_t *check_expr(checker_t *w, expr_t *expr);

static void check_type(checker_t *w, expr_t *expr) {
    assert(expr);
    switch (expr->type) {

    case ast_EXPR_IDENT:
        scope_resolve(w->topScope, expr);
        break;

    case ast_TYPE_ARRAY:
        check_type(w, expr->array.elt);
        if (expr->array.len) {
            expr_t *len = check_expr(w, expr->array.len);
            (void)len; // TODO assert that len resolves to int
        }
        break;

    case ast_TYPE_ENUM:
        for (int i = 0; expr->enum_.enums[i]; i++) {
            decl_t *decl = expr->enum_.enums[i];
            decl->value.type = expr;
            printlg("check_type: declaring %s", decl->value.name->ident.name);
            scope_declare(w->topScope, decl);
        }
        break;

    case ast_TYPE_FUNC:
        for (int i = 0; expr->func.params && expr->func.params[i]; i++) {
            decl_t *param = expr->func.params[i];
            assert(param->type == ast_DECL_FIELD);
            if (param->field.type) {
                check_type(w, param->field.type);
            }
        }
        if (expr->func.result) {
            check_type(w, expr->func.result);
        }
        break;

    case ast_EXPR_STAR:
        check_type(w, expr->star.x);
        break;

    case ast_TYPE_QUAL:
        check_type(w, expr->qual.type);
        break;

    case ast_TYPE_STRUCT:
        if (expr->struct_.fields) {
            checker_openScope(w);
            for (int i = 0; expr->struct_.fields[i]; i++) {
                decl_t *field = expr->struct_.fields[i];
                check_type(w, field->field.type);
                if (field->field.name) {
                    printlg("check_type: declaring %s",
                            field->field.name->ident.name);
                    scope_declare(w->topScope, field);
                } else {
                    print("anonymous struct");
                }
            }
            checker_closeScope(w);
        }
        break;

    default:
        panic("check_type: unknown type: %s", types_typeString(expr));
        break;
    }
}

static bool types_isLhs(expr_t *expr) {
    switch (expr->type) {
    case ast_EXPR_IDENT:
    case ast_EXPR_SELECTOR:
        return true;
    case ast_EXPR_CAST:
        return types_isLhs(expr->cast.expr);
    case ast_EXPR_INDEX:
        return types_isLhs(expr->index.x);
    case ast_EXPR_PAREN:
        return types_isLhs(expr->paren.x);
    case ast_EXPR_STAR:
        return types_isLhs(expr->star.x);
    default:
        return false;
    }
}

static expr_t *get_decl_type(decl_t *decl) {
    assert(decl);
    switch (decl->type) {
    case ast_DECL_FIELD:
        return decl->field.type;
    case ast_DECL_FUNC:
        return decl->func.type;
    case ast_DECL_TYPEDEF:
        return decl->typedef_.type;
    case ast_DECL_VALUE:
        return decl->value.type;
    default:
        panic("unhandled decl: %d", decl->type);
        return NULL;
    }
}


static expr_t *get_ident_type(expr_t *ident) {
    assert(ident->type == ast_EXPR_IDENT);
    object_t *obj = ident->ident.obj;
    assert(obj);
    return get_decl_type(obj->decl);
}

static expr_t *find_field(expr_t *type, expr_t *sel) {
    assert(type->type == ast_TYPE_STRUCT);
    for (int i = 0; type->struct_.fields && type->struct_.fields[i]; i++) {
        decl_t *field = type->struct_.fields[i];
        expr_t *name = field->field.name;
        if (name) {
            printlg("field: %s == %s?", name->ident.name, sel->ident.name);
            if (streq(sel->ident.name, name->ident.name)) {
                type = field->field.type;
                return type;
            }
        } else {
            return find_field(field->field.type, sel);
        }
    }
    return NULL;
}

static expr_t *check_expr(checker_t *w, expr_t *expr) {
    assert(expr);
    switch (expr->type) {

    case ast_EXPR_BINARY:
        {
            expr_t *typ1 = check_expr(w, expr->binary.x);
            expr_t *typ2 = check_expr(w, expr->binary.y);
            if (!types_areComparable(typ1, typ2)) {
                panic("not compariable: %s and %s: %s",
                        types_typeString(typ1),
                        types_typeString(typ2),
                        types_exprString(expr));
            }
            switch (expr->binary.op) {
            case token_EQUAL:
            case token_GT:
            case token_GT_EQUAL:
            case token_LT:
            case token_LT_EQUAL:
            case token_NOT_EQUAL:
                typ1 = types_makeIdent("bool");
                check_type(w, typ1);
            default:
                return typ1;
            }
        }

    case ast_EXPR_BASIC_LIT:
        {
            token_t kind = expr->basic_lit.kind;
            expr_t *type = NULL;
            switch (kind) {
            case token_CHAR: type = types_makeIdent("char"); break;
            case token_INT: type = types_makeIdent("int"); break;
            case token_FLOAT: type = types_makeIdent("float"); break;
            case token_STRING: type = types_makePtr(types_makeIdent("char")); break;
            default:
                panic("check_expr: not implmented: %s", token_string(kind));
                break;
            }
            check_type(w, type);
            return type;
        }

    case ast_EXPR_COMPOUND:
        return NULL;

    case ast_EXPR_CALL:
        {
            expr_t *func = expr->call.func;
            expr_t *type = check_expr(w, func);
            if (func->type == ast_EXPR_IDENT) {
                if (streq(func->ident.name, "esc")) {
                    assert(expr->call.args[0] && !expr->call.args[1]);
                    expr_t *type = check_expr(w, expr->call.args[0]);
                    return types_makePtr(type);
                }
            }
            if (type->type == ast_EXPR_STAR) {
                type = type->star.x;
            }
            if (type->type != ast_TYPE_FUNC) {
                panic("check_expr: `%s` is not a func: %s",
                        types_exprString(expr->call.func),
                        types_exprString(expr));
            }
            int j = 0;
            for (int i = 0; expr->call.args[i]; i++) {
                decl_t *param = type->func.params[j];
                assert(param->type == ast_DECL_FIELD);
                expr_t *type = check_expr(w, expr->call.args[i]);
                if (param->field.type) {
                    if (!types_areAssignable(param->field.type, type)) {
                        panic("not assignable: %s and %s: %s",
                                types_typeString(param->field.type),
                                types_typeString(type),
                                types_exprString(expr));
                    }
                    j++;
                }
            }
            return type->func.result;

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
            return get_ident_type(expr);
        }

    case ast_EXPR_INDEX:
        {
            expr_t *type = check_expr(w, expr->index.x);
            switch (type->type) {
            case ast_TYPE_ARRAY:
                type = type->array.elt;
                break;
            case ast_EXPR_STAR:
                type = type->star.x;
                break;
            default:
                panic("indexing a non-array or pointer `%s`: %s",
                        types_typeString(type),
                        types_exprString(expr));
                break;
            }
            expr_t *typ2 = check_expr(w, expr->index.index);
            (void)typ2; // TODO assert that typ2 is an integer
            return type;
        }

    case ast_EXPR_PAREN:
        return check_expr(w, expr->paren.x);

    case ast_EXPR_SELECTOR:
        {
            expr_t *type = check_expr(w, expr->selector.x);
            assert(type);
            if (type->type == ast_EXPR_STAR) {
                expr->selector.tok = token_ARROW;
                type = type->star.x;
            } else {
                assert(expr->selector.tok != token_ARROW);
            }
            type = unwind_typedef(type);
            printlg("selector: %s", expr->selector.sel->ident.name);
            expr_t *eType = find_field(type, expr->selector.sel);
            if (eType == NULL) {
                panic("struct `%s` (`%s`) has no field `%s`",
                        types_exprString(expr->selector.x),
                        types_typeString(type),
                        expr->selector.sel->ident.name);
            }
            return eType;
        }

    case ast_EXPR_SIZEOF:
        {
            check_type(w, expr->sizeof_.x);
            expr_t *ident = types_makeIdent("size_t");
            scope_resolve(w->topScope, ident);
            return ident;
        }

    case ast_EXPR_STAR:
        {
            expr_t *type = check_expr(w, expr->star.x);
            if (type->type != ast_EXPR_STAR) {
                panic("check_expr: deferencing a non-pointer `%s`: %s",
                        types_typeString(type),
                        types_exprString(expr));
            }
            return type->star.x;
        }

    case ast_EXPR_UNARY:
        {
            expr_t *type = check_expr(w, expr->unary.x);
            if (expr->unary.op == token_AND) {
                if (!types_isLhs(expr->unary.x)) {
                    panic("check_expr: invalid lvalue `%s`: %s",
                            types_exprString(expr->unary.x),
                            types_exprString(expr));
                }
                return types_makePtr(type);
            }
            return type;
        }

    default:
        panic("check_expr: unknown expr: %s", types_exprString(expr));
    }
    return NULL;
}

static void check_decl(checker_t *w, decl_t *decl);

static void check_stmt(checker_t *w, stmt_t *stmt) {
    switch (stmt->type) {
    case ast_STMT_ASSIGN:
        {
            if (!types_isLhs(stmt->assign.x)) {
                panic("check_stmt: invalid lvalue `%s`: %s",
                        types_exprString(stmt->assign.x),
                        types_stmtString(stmt));
            }
            expr_t *a = check_expr(w, stmt->assign.x);
            if (a->type == ast_TYPE_QUAL) {
                panic("cannot assign to const var: %s", types_stmtString(stmt));
            }
            expr_t *b = check_expr(w, stmt->assign.y);
            if (b->type == ast_TYPE_QUAL) {
                b = b->qual.type;
            }
            if (!types_areAssignable(a, b)) {
                panic("check_stmt: not assignment `%s` and `%s`: %s",
                        types_exprString(stmt->assign.x),
                        types_exprString(stmt->assign.y),
                        types_stmtString(stmt));
            }
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
        /* TODO walk label in label scope */
        break;

    case ast_STMT_LABEL:
        /* TODO walk label in label scope */
        check_stmt(w, stmt->label.stmt);
        break;

    case ast_STMT_POSTFIX:
        check_expr(w, stmt->postfix.x);
        break;

    case ast_STMT_RETURN:
        if (stmt->return_.x) {
            expr_t *a = w->result;
            assert(a);
            expr_t *b = check_expr(w, stmt->return_.x);
            if (a->type == ast_TYPE_QUAL) {
                a = a->qual.type;
            }
            if (b->type == ast_TYPE_QUAL) {
                b = b->qual.type;
            }
            if (!types_areAssignable(a, b)) {
                panic("check_stmt: not returnable: %s and %s: %s",
                        types_typeString(a),
                        types_typeString(b),
                        types_stmtString(stmt));
            }
        }
        break;

    case ast_STMT_SWITCH:
        {
            expr_t *type1 = check_expr(w, stmt->switch_.tag);
            for (int i = 0; stmt->switch_.stmts[i]; i++) {
                stmt_t *clause = stmt->switch_.stmts[i];
                assert(clause->type == ast_STMT_CASE);
                if (clause->case_.expr) {
                    expr_t *type2 = check_expr(w, clause->case_.expr);
                    if (!types_areComparable(type1, type2)) {
                        panic("check_stmt: not comparable: %s and %s: %s",
                                types_typeString(type1),
                                types_typeString(type2),
                                types_stmtString(stmt));
                    }
                }
                for (int j = 0; clause->case_.stmts[j]; j++) {
                    check_stmt(w, clause->case_.stmts[j]);
                }
            }
        }
        break;

    default:
        panic("check_stmt: unknown stmt: %s", types_stmtString(stmt));
    }
}

static void check_decl(checker_t *w, decl_t *decl) {
    switch (decl->type) {
    case ast_DECL_FUNC:
        check_type(w, decl->func.type);
        printlg("check_decl: declaring %s", decl->func.name->ident.name);
        scope_declare(w->topScope, decl);
        break;
    case ast_DECL_IMPORT:
        break;
    case ast_DECL_TYPEDEF:
        check_type(w, decl->typedef_.type);
        printlg("check_decl: declaring %s", decl->typedef_.name->ident.name);
        scope_declare(w->topScope, decl);
        break;
    case ast_DECL_VALUE:
        {
            expr_t *b = NULL;
            if (decl->value.type != NULL) {
                check_type(w, decl->value.type);
            }
            if (decl->value.value != NULL) {
                b = check_expr(w, decl->value.value);
            }
            if (decl->value.type == NULL) {
                decl->value.type = b;
            }
            if (b != NULL) {
                if (b->type == ast_TYPE_QUAL) {
                    b = b->qual.type;
                }
                expr_t *a = decl->value.type;
                if (!types_areAssignable(a, b)) {
                    panic("check_decl: not assignable %s and %s: %s",
                            types_typeString(a),
                            types_typeString(b),
                            types_declString(decl));
                }
            }
            printlg("check_decl: declaring %s", decl->value.name->ident.name);
            scope_declare(w->topScope, decl);
            break;
        }
    default:
        panic("check_decl: not implemented: %d", decl->type);
    }
}

static void check_func(checker_t *w, decl_t *decl) {
    if (decl->type == ast_DECL_FUNC && decl->func.body) {
        printlg("check_func: walking func %s", decl->func.name->ident.name);
        checker_openScope(w);
        expr_t *type = decl->func.type;
        for (int i = 0; type->func.params && type->func.params[i]; i++) {
            decl_t *param = type->func.params[i];
            assert(param->type == ast_DECL_FIELD);
            if (param->field.name) {
                printlg("check_func: declaring param `%s`",
                        param->field.name->ident.name);
                scope_declare(w->topScope, param);
            }
        }
        w->result = decl->func.type->func.result;
        // walk the block manually to avoid opening a new scope
        for (int i = 0; decl->func.body->block.stmts[i]; i++) {
            check_stmt(w, decl->func.body->block.stmts[i]);
        }
        w->result = NULL;
        checker_closeScope(w);
    }
}

extern void types_checkFile(file_t *file) {
    printlg("checking file %s", file->filename);
    checker_t w = {.topScope = file->scope};
    for (int i = 0; file->decls[i] != NULL; i++) {
        check_decl(&w, file->decls[i]);
    }
    for (int i = 0; file->decls[i] != NULL; i++) {
        check_func(&w, file->decls[i]);
    }
}
