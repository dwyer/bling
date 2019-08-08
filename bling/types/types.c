#include "bling/ast/ast.h"
#include "bling/emitter/emitter.h"
#include "bling/parser/parser.h"
#include "bling/types/types.h"
#include "path/path.h"

static struct {
    char *name;
    int size;
    bool arith;
} natives[] = {
    // native types
    {"char", sizeof(char), true},
    {"double", sizeof(double), true},
    {"float", sizeof(float), true},
    {"int", sizeof(int), true},
    {"void", sizeof(void)},
    // libc types
    {"int16_t", sizeof(int16_t), true},
    {"int32_t", sizeof(int32_t), true},
    {"int64_t", sizeof(int64_t), true},
    {"int8_t", sizeof(int8_t), true},
    {"size_t", sizeof(size_t), true},
    {"uint16_t", sizeof(uint16_t), true},
    {"uint32_t", sizeof(uint32_t), true},
    {"uint64_t", sizeof(uint64_t), true},
    {"uint8_t", sizeof(uint8_t), true},
    {"uintptr_t", sizeof(uintptr_t), true},
    // sentinel
    {NULL},
};

extern char *types_declString(ast$Decl *decl) {
    emitter_t e = {};
    print_decl(&e, decl);
    return emitter_string(&e);
}

extern char *types_exprString(ast$Expr *expr) {
    emitter_t e = {};
    print_expr(&e, expr);
    return emitter_string(&e);
}

extern char *types_stmtString(ast$Stmt *stmt) {
    emitter_t e = {};
    print_stmt(&e, stmt);
    return emitter_string(&e);
}

extern char *types_typeString(ast$Expr *expr) {
    emitter_t e = {};
    print_type(&e, expr);
    return emitter_string(&e);
}

extern bool types_isType(ast$Expr *expr) {
    switch (expr->type) {
    case ast$EXPR_IDENT:
        if (expr->ident.obj == NULL) {
            panic("types_isType: unresolved identifier %s", expr->ident.name);
        }
        return expr->ident.obj->decl->type == ast$DECL_TYPEDEF;
    case ast$EXPR_STAR:
        return types_isType(expr->star.x);
    case ast$TYPE_ARRAY:
    case ast$TYPE_ENUM:
    case ast$TYPE_FUNC:
    case ast$TYPE_NATIVE:
    case ast$TYPE_STRUCT:
        return true;
    default:
        return false;
    }
}

static ast$Expr *types_makeIdent(const char *name) {
    ast$Expr x = {
        .type = ast$EXPR_IDENT,
        .ident = {
            .name = strdup(name),
        },
    };
    return esc(x);
}

static ast$Expr *types_makePtr(ast$Expr *type) {
    ast$Expr x = {
        .type = ast$EXPR_STAR,
        .pos = type->pos,
        .star = {
            .x = type,
        }
    };
    return esc(x);
}

static ast$Expr *lookup_typedef(ast$Expr *ident) {
    ast$Decl *decl = ident->ident.obj->decl;
    if (decl->type != ast$DECL_TYPEDEF) {
        panic("not a typedef: %s", types_typeString(ident));
    }
    return decl->typedef_.type;
}

static ast$Expr *types_getBaseType(ast$Expr *type) {
    for (;;) {
        switch (type->type) {
        case ast$EXPR_IDENT:
            type = lookup_typedef(type);
            break;
        case ast$TYPE_ARRAY:
        case ast$TYPE_ENUM:
        case ast$TYPE_NATIVE:
        case ast$TYPE_STRUCT:
            return type;
        default:
            panic("not a typedef: %s", types_typeString(type));
        }
    }
}

static bool types_isArithmetic(ast$Expr *type) {
    switch (type->type) {
    case ast$EXPR_IDENT:
        return types_isArithmetic(types_getBaseType(type));
    case ast$EXPR_STAR:
    case ast$TYPE_ENUM:
        return true;
    case ast$TYPE_NATIVE:
        return !streq(type->native.name, "void");
    default:
        return false;
    }
}

static bool types_isNative(ast$Expr *type, const char *name) {
    switch (type->type) {
    case ast$EXPR_IDENT:
        return streq(type->ident.name, name);
    case ast$TYPE_NATIVE:
        return true;
    default:
        return false;
    }
}

static bool types_areIdentical(ast$Expr *a, ast$Expr *b) {
    if (a == b) {
        return true;
    }
    if (a == NULL || b == NULL) {
        return false;
    }
    assert(a);
    assert(b);
    if (a->type != b->type) {
        return false;
    }
    switch (a->type) {
    case ast$EXPR_IDENT:
        return b->type == ast$EXPR_IDENT && a->ident.obj == a->ident.obj;
    case ast$EXPR_STAR:
        if (ast$isVoidPtr(a) || ast$isVoidPtr(b)) {
            return true;
        }
        return types_areIdentical(a->star.x, b->star.x);
    case ast$TYPE_ARRAY:
        // TODO check lengths
        return types_areIdentical(a->array.elt, b->array.elt);
    case ast$TYPE_FUNC:
        {
            if (!types_areIdentical(a->func.result, b->func.result)) {
                return false;
            }
            int i = 0;
            if (a->func.params) {
                for (; a->func.params[i]; i++) {
                    ast$Decl *param1 = a->func.params[i];
                    ast$Decl *param2 = b->func.params[i];
                    if (param2 == NULL) {
                        return false;
                    }
                    if (!types_areIdentical(param1->field.type, param2->field.type)) {
                        return false;
                    }
                }
                if (b->func.params[i]) {
                    return false;
                }
            }
        }
        return true;
    default:
        panic("unreachable: %s == %s", types_typeString(a), types_typeString(b));
        return false;
    }
}

static bool types_isPointer(ast$Expr *t) {
    return t->type == ast$EXPR_STAR || t->type == ast$TYPE_ARRAY;
}

static ast$Expr *types_pointerBase(ast$Expr *t) {
    switch (t->type) {
    case ast$EXPR_STAR:
        return t->star.x;
    case ast$TYPE_ARRAY:
        return t->array.elt;
    default:
        panic("not a pointer: %s", types_typeString(t));
        return NULL;
    }
}

static bool types_areAssignable(ast$Expr *a, ast$Expr *b) {
    if (types_areIdentical(a, b)) {
        return true;
    }
    if (ast$isVoidPtr(a) || ast$isVoidPtr(b)) {
        return true;
    }
    if (types_isPointer(a) && types_isPointer(b)) {
        return types_areAssignable(types_pointerBase(a), types_pointerBase(b));
    }
    while (a->type == ast$EXPR_IDENT) {
        a = lookup_typedef(a);
    }
    while (b->type == ast$EXPR_IDENT) {
        b = lookup_typedef(b);
    }
    if (types_isNative(a, "bool") && types_isArithmetic(b)) {
        return true;
    }
    if (b->type == ast$TYPE_ENUM && types_isArithmetic(a)) {
        return true;
    }
    if (a->type == ast$TYPE_ENUM && types_isArithmetic(a)) {
        return true;
    }
    return types_areIdentical(a, b);
}

static bool types_areComparable(ast$Expr *a, ast$Expr *b) {
    if (types_areIdentical(a, b)) {
        return true;
    }
    if (types_isArithmetic(a) && types_isArithmetic(b)) {
        return true;
    }
    if (a->type == ast$EXPR_STAR && types_isNative(b, "int")) {
        return true;
    }
    return types_areIdentical(a, b);
}

static void ast$Scope_declare(ast$Scope *s, ast$Decl *decl) {
    ast$ObjKind kind;
    ast$Expr *ident = NULL;
    switch (decl->type) {
    case ast$DECL_FIELD:
        kind = ast$ObjKind_VALUE;
        ident = decl->field.name;
        break;
    case ast$DECL_FUNC:
        kind = ast$ObjKind_FUNC;
        ident = decl->func.name;
        break;
    case ast$DECL_IMPORT:
        kind = ast$ObjKind_PKG;
        ident = decl->imp.name;
        break;
    case ast$DECL_TYPEDEF:
        kind = ast$ObjKind_TYPE;
        ident = decl->typedef_.name;
        break;
    case ast$DECL_VALUE:
        kind = ast$ObjKind_VALUE;
        ident = decl->value.name;
        break;
    default:
        panic("ast$Scope_declare: bad decl: %d", decl->type);
        return;
    }
    assert(ident->type == ast$EXPR_IDENT);
    if (ident->ident.obj != NULL) {
        panic("already declared: %s", ident->ident.name);
    }
    ast$Object *obj = object_new(kind, ident->ident.name);
    obj->decl = decl;
    obj->pkg = s->pkg;
    ident->ident.obj = obj;
    ast$Object *alt = ast$Scope_insert(s, obj);
    if (alt != NULL) {
        if (alt->kind != kind) {
            panic("incompatible redefinition of `%s`: %s",
                    types_declString(obj->decl),
                    types_declString(decl));
        }
        bool redecl = false;
        switch (kind) {
        case ast$ObjKind_FUNC:
            redecl =
                (alt->decl->func.body == NULL || decl->func.body == NULL) &&
                types_areIdentical(alt->decl->func.type, decl->func.type);
            break;
        case ast$ObjKind_PKG:
            redecl = true;
            break;
        case ast$ObjKind_TYPE:
            if (alt->decl->typedef_.type->type == ast$TYPE_STRUCT) {
                redecl = alt->decl->typedef_.type->struct_.fields == NULL;
            }
            break;
        case ast$ObjKind_VALUE:
            redecl = alt->decl->value.value == NULL &&
                !types_areIdentical(alt->decl->value.type, decl->value.value);
            break;
        default:
            panic("unknown kind: %d", kind);
            break;
        }
        if (!redecl) {
            panic("already declared: %s", ident->ident.name);
        }
        alt->decl = decl;
    }
}

static void declare_builtins(ast$Scope *s) {
    for (int i = 0; natives[i].name != NULL; i++) {
        ast$Expr name = {
            .type = ast$EXPR_IDENT,
            .ident = {
                .name = strdup(natives[i].name),
            },
        };
        ast$Expr type = {
            .type = ast$TYPE_NATIVE,
            .native = {
                .name = strdup(natives[i].name),
                .size = natives[i].size,
            },
        };
        ast$Decl decl = {
            .type = ast$DECL_TYPEDEF,
            .typedef_ = {
                .name = esc(name),
                .type = esc(type),
            },
        };
        ast$Scope_declare(s, esc(decl));
    }
}

ast$Scope *_universe = NULL;

extern ast$Scope *types_universe() {
    if (_universe == NULL) {
        _universe = ast$Scope_new(NULL);
        declare_builtins(_universe);
        ast$File *file = parser_parse_file("builtin/builtin.bling");
        file->scope = _universe;
        config_t conf = {.strict = true};
        types_checkFile(&conf, file);
        free(file->decls);
        free(file);
    }
    return _universe;
}

typedef struct {
    config_t *conf;
    ast$Package pkg;
    ast$Expr *result;
    slice$Slice files;
    ast$Expr *typedefName;
    map$Map scopes;
} checker_t;

static void checker_openScope(checker_t *w) {
    w->pkg.scope = ast$Scope_new(w->pkg.scope);
}

static void checker_closeScope(checker_t *w) {
    ast$Scope *inner = w->pkg.scope;
    w->pkg.scope = w->pkg.scope->outer;
    ast$Scope_free(inner);
}

static ast$Expr *check_expr(checker_t *w, ast$Expr *expr);

static void check_type(checker_t *w, ast$Expr *expr) {
    assert(expr);
    switch (expr->type) {

    case ast$EXPR_IDENT:
        ast$Scope_resolve(w->pkg.scope, expr);
        break;

    case ast$TYPE_ARRAY:
        check_type(w, expr->array.elt);
        if (expr->array.len) {
            ast$Expr *len = check_expr(w, expr->array.len);
            (void)len; // TODO assert that len resolves to int
        }
        break;

    case ast$TYPE_ENUM:
        for (int i = 0; expr->enum_.enums[i]; i++) {
            ast$Decl *decl = expr->enum_.enums[i];
            if (w->typedefName) {
                decl->value.type = w->typedefName;
            }
            ast$Scope_declare(w->pkg.scope, decl);
        }
        break;

    case ast$TYPE_FUNC:
        for (int i = 0; expr->func.params && expr->func.params[i]; i++) {
            ast$Decl *param = expr->func.params[i];
            assert(param->type == ast$DECL_FIELD);
            if (param->field.type) {
                check_type(w, param->field.type);
            }
        }
        if (expr->func.result) {
            check_type(w, expr->func.result);
        }
        break;

    case ast$EXPR_STAR:
        check_type(w, expr->star.x);
        break;

    case ast$TYPE_STRUCT:
        if (expr->struct_.fields) {
            checker_openScope(w);
            for (int i = 0; expr->struct_.fields[i]; i++) {
                ast$Decl *field = expr->struct_.fields[i];
                check_type(w, field->field.type);
                if (field->field.name) {
                    ast$Scope_declare(w->pkg.scope, field);
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

static bool types_isLhs(ast$Expr *expr) {
    switch (expr->type) {
    case ast$EXPR_IDENT:
    case ast$EXPR_SELECTOR:
        return true;
    case ast$EXPR_CAST:
        return types_isLhs(expr->cast.expr);
    case ast$EXPR_INDEX:
        return types_isLhs(expr->index.x);
    case ast$EXPR_PAREN:
        return types_isLhs(expr->paren.x);
    case ast$EXPR_STAR:
        return types_isLhs(expr->star.x);
    default:
        return false;
    }
}

static ast$Expr *get_ast$Declype(ast$Decl *decl) {
    assert(decl);
    switch (decl->type) {
    case ast$DECL_FIELD:
        return decl->field.type;
    case ast$DECL_FUNC:
        return decl->func.type;
    case ast$DECL_TYPEDEF:
        return decl->typedef_.type;
    case ast$DECL_VALUE:
        return decl->value.type;
    default:
        panic("unhandled decl: %d", decl->type);
        return NULL;
    }
}


static ast$Expr *get_ident_type(ast$Expr *ident) {
    assert(ident->type == ast$EXPR_IDENT);
    ast$Object *obj = ident->ident.obj;
    assert(obj);
    return get_ast$Declype(obj->decl);
}

static ast$Decl *find_field(ast$Expr *type, ast$Expr *sel) {
    assert(type->type == ast$TYPE_STRUCT);
    assert(sel->type == ast$EXPR_IDENT);
    for (int i = 0; type->struct_.fields && type->struct_.fields[i]; i++) {
        ast$Decl *field = type->struct_.fields[i];
        ast$Expr *name = field->field.name;
        if (name) {
            if (streq(sel->ident.name, name->ident.name)) {
                type = field->field.type;
                return field;
            }
        } else {
            return find_field(field->field.type, sel);
        }
    }
    return NULL;
}

static bool types_isInteger(ast$Expr *x) {
    switch (x->type) {
    case ast$EXPR_IDENT:
        return types_isInteger(x->ident.obj->decl->typedef_.type);
    case ast$TYPE_ENUM:
    case ast$TYPE_NATIVE:
        return true;
    default:
        return false;
    }
}

static void check_array(checker_t *w, ast$Expr *x) {
    ast$Expr *baseT = types_getBaseType(x->compound.type);
    for (int i = 0; x->compound.list[i]; i++) {
        ast$Expr *elt = x->compound.list[i];
        if (elt->type == ast$EXPR_KEY_VALUE) {
            elt->key_value.isArray = true;
            ast$Expr *indexT = check_expr(w, elt->key_value.key);
            if (!types_isInteger(indexT)) {
                panic("not a valid index: %s: %s",
                        types_exprString(elt->key_value.key),
                        types_exprString(elt));
            }
            elt = elt->key_value.value;
        }
        if (elt->type == ast$EXPR_COMPOUND) {
            if (elt->compound.type == NULL) {
                elt->compound.type = baseT->array.elt;
            }
        }
        ast$Expr *eltT = check_expr(w, elt);
        (void)eltT;
    }
}

static ast$Decl *getStructField(ast$Expr *type, int index) {
    assert(type->type == ast$TYPE_STRUCT);
    if (type->struct_.fields == NULL) {
        panic("incomplete field defn: %s", types_typeString(type));
    }
    for (int i = 0; type->struct_.fields[i]; i++) {
        if (i == index) {
            return type->struct_.fields[i];
        }
    }
    return NULL;
}

static void check_struct(checker_t *w, ast$Expr *x) {
    assert(x->compound.type);
    ast$Expr *baseT = types_getBaseType(x->compound.type);
    bool expectKV = false;
    for (int i = 0; x->compound.list[i]; i++) {
        ast$Expr *elt = x->compound.list[i];
        ast$Expr *fieldT = NULL;
        if (elt->type == ast$EXPR_KEY_VALUE) {
            elt->key_value.isArray = false;
            expectKV = true;
            ast$Expr *key = elt->key_value.key;
            if (!ast$isIdent(key)) {
                panic("key must be an identifier: %s",
                        types_typeString(x->compound.type));
            }
            ast$Decl *field = find_field(baseT, key);
            if (field == NULL) {
                panic("struct `%s` has no field `%s`",
                        types_typeString(x->compound.type),
                        types_exprString(key));
            }
            key->ident.obj = field->field.name->ident.obj;
            fieldT = field->field.type;
            elt = elt->key_value.value;
        } else {
            if (expectKV) {
                panic("expected a key/value expr: %s", types_exprString(x));
            }
            ast$Decl *field = getStructField(baseT, i);
            fieldT = field->field.type;
        }
        if (elt->type == ast$EXPR_COMPOUND) {
            if (elt->compound.type == NULL) {
                elt->compound.type = fieldT;
            }
        }
        ast$Expr *eltT = check_expr(w, elt);
        if (!types_areAssignable(fieldT, eltT)) {
            panic("cannot init field of type `%s` with value of type `%s`: %s",
                    types_typeString(fieldT),
                    types_typeString(eltT),
                    types_exprString(elt));
        }
    }
}

static void check_compositeLit(checker_t *w, ast$Expr *x) {
    ast$Expr *t = x->compound.type;
    assert(t);
    ast$Expr *baseT = types_getBaseType(t);
    switch (baseT->type) {
    case ast$TYPE_ARRAY:
        check_array(w, x);
        break;
    case ast$TYPE_STRUCT:
        check_struct(w, x);
        break;
    default:
        panic("composite type must be an array or a struct: %s",
                types_typeString(t));
    }
}

static ast$Expr *check_expr(checker_t *w, ast$Expr *expr) {
    assert(expr);
    switch (expr->type) {

    case ast$EXPR_BINARY:
        {
            ast$Expr *typ1 = check_expr(w, expr->binary.x);
            ast$Expr *typ2 = check_expr(w, expr->binary.y);
            if (!types_areComparable(typ1, typ2)) {
                panic("not compariable: %s and %s: %s",
                        types_typeString(typ1),
                        types_typeString(typ2),
                        types_exprString(expr));
            }
            switch (expr->binary.op) {
            case token$EQUAL:
            case token$GT:
            case token$GT_EQUAL:
            case token$LAND:
            case token$LOR:
            case token$LT:
            case token$LT_EQUAL:
            case token$NOT_EQUAL:
                typ1 = types_makeIdent("bool");
                check_type(w, typ1);
            default:
                return typ1;
            }
        }

    case ast$EXPR_BASIC_LIT:
        {
            token$Token kind = expr->basic_lit.kind;
            ast$Expr *type = NULL;
            switch (kind) {
            case token$CHAR:
                type = types_makeIdent("char");
                break;
            case token$FLOAT:
                type = types_makeIdent("float");
                break;
            case token$INT:
                type = types_makeIdent("int");
                break;
            case token$STRING:
                type = types_makePtr(types_makeIdent("char"));
                break;
            default:
                panic("check_expr: not implmented: %s", token$string(kind));
                break;
            }
            check_type(w, type);
            return type;
        }

    case ast$EXPR_CALL:
        {
            ast$Expr *func = expr->call.func;
            ast$Expr *type = check_expr(w, func);
            if (func->type == ast$EXPR_IDENT) {
                if (streq(func->ident.name, "esc")) {
                    assert(expr->call.args[0] && !expr->call.args[1]);
                    ast$Expr *type = check_expr(w, expr->call.args[0]);
                    return types_makePtr(type);
                }
            }
            if (type->type == ast$EXPR_STAR) {
                type = type->star.x;
            }
            if (type->type != ast$TYPE_FUNC) {
                panic("check_expr: `%s` is not a func: %s",
                        types_exprString(expr->call.func),
                        types_exprString(expr));
            }
            int j = 0;
            for (int i = 0; expr->call.args[i]; i++) {
                ast$Decl *param = type->func.params[j];
                assert(param->type == ast$DECL_FIELD);
                ast$Expr *type = check_expr(w, expr->call.args[i]);
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

    case ast$EXPR_COMPOUND:
        if (expr->compound.type == NULL) {
            panic("untyped compound expr: %s", types_exprString(expr));
        }
        check_compositeLit(w, expr);
        return expr->compound.type;

    case ast$EXPR_CAST:
        {
            check_type(w, expr->cast.type);
            if (expr->cast.expr->type == ast$EXPR_COMPOUND) {
                expr->cast.expr->compound.type = expr->cast.type;
            }
            check_expr(w, expr->cast.expr);
            return expr->cast.type;
        }

    case ast$EXPR_IDENT:
        {
            ast$Scope_resolve(w->pkg.scope, expr);
            return get_ident_type(expr);
        }

    case ast$EXPR_INDEX:
        {
            ast$Expr *type = check_expr(w, expr->index.x);
            switch (type->type) {
            case ast$TYPE_ARRAY:
                type = type->array.elt;
                break;
            case ast$EXPR_STAR:
                type = type->star.x;
                break;
            default:
                panic("indexing a non-array or pointer `%s`: %s",
                        types_typeString(type),
                        types_exprString(expr));
                break;
            }
            ast$Expr *typ2 = check_expr(w, expr->index.index);
            (void)typ2; // TODO assert that typ2 is an integer
            return type;
        }

    case ast$EXPR_PAREN:
        return check_expr(w, expr->paren.x);

    case ast$EXPR_SELECTOR:
        {
            ast$Expr *type = check_expr(w, expr->selector.x);
            if (type->type == ast$EXPR_STAR) {
                expr->selector.tok = token$ARROW;
                type = type->star.x;
            } else {
                assert(expr->selector.tok != token$ARROW);
            }
            type = types_getBaseType(type);
            ast$Decl *field = find_field(type, expr->selector.sel);
            if (field == NULL) {
                panic("struct `%s` (`%s`) has no field `%s`",
                        types_exprString(expr->selector.x),
                        types_typeString(type),
                        expr->selector.sel->ident.name);
            }
            expr->selector.sel->ident.obj = field->field.name->ident.obj;
            return field->field.type;
        }

    case ast$EXPR_SIZEOF:
        {
            check_type(w, expr->sizeof_.x);
            ast$Expr *ident = types_makeIdent("size_t");
            ast$Scope_resolve(w->pkg.scope, ident);
            return ident;
        }

    case ast$EXPR_STAR:
        {
            ast$Expr *type = check_expr(w, expr->star.x);
            switch (type->type) {
            case ast$EXPR_STAR:
                return type->star.x;
            case ast$TYPE_ARRAY:
                return type->array.elt;
            default:
                panic("check_expr: derefencing a non-pointer `%s`: %s",
                        types_typeString(type),
                        types_exprString(expr));
                return NULL;
            }
        }

    case ast$EXPR_UNARY:
        {
            ast$Expr *type = check_expr(w, expr->unary.x);
            if (expr->unary.op == token$AND) {
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

static void check_decl(checker_t *w, ast$Decl *decl);

static void check_stmt(checker_t *w, ast$Stmt *stmt) {
    switch (stmt->type) {
    case ast$STMT_ASSIGN:
        {
            if (!types_isLhs(stmt->assign.x)) {
                panic("check_stmt: invalid lvalue `%s`: %s",
                        types_exprString(stmt->assign.x),
                        types_stmtString(stmt));
            }
            ast$Expr *a = check_expr(w, stmt->assign.x);
            if (a->is_const) {
                panic("cannot assign to const var: %s", types_stmtString(stmt));
            }
            ast$Expr *b = check_expr(w, stmt->assign.y);
            if (!types_areAssignable(a, b)) {
                panic("check_stmt: not assignment `%s` and `%s`: %s",
                        types_exprString(stmt->assign.x),
                        types_exprString(stmt->assign.y),
                        types_stmtString(stmt));
            }
        }
        break;

    case ast$STMT_BLOCK:
        checker_openScope(w);
        for (int i = 0; stmt->block.stmts[i]; i++) {
            check_stmt(w, stmt->block.stmts[i]);
        }
        checker_closeScope(w);
        break;

    case ast$STMT_DECL:
        check_decl(w, stmt->decl);
        break;

    case ast$STMT_EXPR:
        check_expr(w, stmt->expr.x);
        break;

    case ast$STMT_EMPTY:
        break;

    case ast$STMT_IF:
        check_expr(w, stmt->if_.cond);
        check_stmt(w, stmt->if_.body);
        if (stmt->if_.else_) {
            check_stmt(w, stmt->if_.else_);
        }
        break;

    case ast$STMT_ITER:
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

    case ast$STMT_JUMP:
        /* TODO walk label in label scope */
        break;

    case ast$STMT_LABEL:
        /* TODO walk label in label scope */
        check_stmt(w, stmt->label.stmt);
        break;

    case ast$STMT_POSTFIX:
        check_expr(w, stmt->postfix.x);
        break;

    case ast$STMT_RETURN:
        if (stmt->return_.x) {
            ast$Expr *a = w->result;
            assert(a);
            ast$Expr *b = check_expr(w, stmt->return_.x);
            if (!types_areAssignable(a, b)) {
                panic("check_stmt: not returnable: %s and %s: %s",
                        types_typeString(a),
                        types_typeString(b),
                        types_stmtString(stmt));
            }
        }
        break;

    case ast$STMT_SWITCH:
        {
            ast$Expr *type1 = check_expr(w, stmt->switch_.tag);
            for (int i = 0; stmt->switch_.stmts[i]; i++) {
                ast$Stmt *clause = stmt->switch_.stmts[i];
                assert(clause->type == ast$STMT_CASE);
                for (int j = 0; clause->case_.exprs && clause->case_.exprs[j]; j++) {
                    ast$Expr *type2 = check_expr(w, clause->case_.exprs[j]);
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

static char *constant_stringVal(ast$Expr *x) {
    // TODO move this to const pkg
    const char *lit = x->basic_lit.value;
    int n = strlen(lit) - 2;
    char *val = malloc(n + 1);
    for (int i = 0; i < n; i++) {
        val[i] = lit[i+1];
    }
    val[n] = '\0';
    return val;
}

static void check_file(checker_t *w, ast$File *file);

static void check_import(checker_t *w, ast$Decl *imp) {
    char *path = constant_stringVal(imp->imp.path);
    if (imp->imp.name == NULL) {
        char *base = path$base(path);
        imp->imp.name = types_makeIdent(base);
    }
    ast$Scope *scope = NULL;
    map$get(&w->scopes, path, &scope);
    if (scope) {
        ast$Scope_declare(w->pkg.scope, imp);
        free(path);
        return;
    }
    scope = w->pkg.scope;
    map$set(&w->scopes, path, &scope);
    error$Error *err = NULL;
    ast$File **files = parser_parseDir(path, &err);
    if (err) {
        panic("%s: %s", path, err->error);
    }
    for (int i = 0; files[i]; i++) {
        check_file(w, files[i]);
    }
}

static void check_decl(checker_t *w, ast$Decl *decl) {
    switch (decl->type) {
    case ast$DECL_FUNC:
        check_type(w, decl->func.type);
        ast$Scope_declare(w->pkg.scope, decl);
        break;
    case ast$DECL_PRAGMA:
        break;
    case ast$DECL_TYPEDEF:
        w->typedefName = decl->typedef_.name;
        check_type(w, decl->typedef_.type);
        w->typedefName = NULL;
        ast$Scope_declare(w->pkg.scope, decl);
        break;
    case ast$DECL_VALUE:
        {
            ast$Expr *valType = NULL;
            if (decl->value.type != NULL) {
                check_type(w, decl->value.type);
            }
            if (decl->value.value != NULL) {
                if (decl->value.value->type == ast$EXPR_COMPOUND) {
                    if (decl->value.type == NULL) {
                        // TODO resolve this restriction by enforcing T{}.
                        panic("cannot assign short var decls with composite type");
                    }
                    decl->value.value->compound.type = decl->value.type;
                }
                valType = check_expr(w, decl->value.value);
            }
            if (decl->value.type == NULL) {
                decl->value.type = valType;
            }
            if (valType != NULL) {
                ast$Expr *varType = decl->value.type;
                if (!types_areAssignable(varType, valType)) {
                    panic("check_decl: not assignable %s and %s: %s",
                            types_typeString(varType),
                            types_typeString(valType),
                            types_declString(decl));
                }
            }
            ast$Scope_declare(w->pkg.scope, decl);
            break;
        }
    default:
        panic("check_decl: not implemented: %s", types_declString(decl));
    }
}

static void check_func(checker_t *w, ast$Decl *decl) {
    if (decl->type == ast$DECL_FUNC && decl->func.body) {
        checker_openScope(w);
        ast$Expr *type = decl->func.type;
        for (int i = 0; type->func.params && type->func.params[i]; i++) {
            ast$Decl *param = type->func.params[i];
            assert(param->type == ast$DECL_FIELD);
            if (param->field.name) {
                ast$Scope_declare(w->pkg.scope, param);
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

static void check_file(checker_t *w, ast$File *file) {
    if (file->name) {
        // checker_openScope(w);
        // w->pkg.scope->pkg = file->name->ident.name;
    }
    for (int i = 0; file->imports[i] != NULL; i++) {
        check_import(w, file->imports[i]);
    }
    slice$append(&w->files, &file);
    if (w->conf->strict) {
        for (int i = 0; file->decls[i] != NULL; i++) {
            check_decl(w, file->decls[i]);
        }
        for (int i = 0; file->decls[i] != NULL; i++) {
            check_func(w, file->decls[i]);
        }
    }
    if (file->name) {
        // checker_closeScope(w);
    }
}

extern ast$Package types_checkFile(config_t *conf, ast$File *file) {
    if (file->scope == NULL) {
        file->scope = ast$Scope_new(types_universe());
    }
    checker_t w = {
        .conf = conf,
        .pkg = {
            .scope = file->scope,
        },
        .files = slice$init(sizeof(ast$File *)),
        .scopes = map$init(sizeof(ast$Scope *)),
    };
    check_file(&w, file);
    w.pkg.files = slice$to_nil_array(w.files);
    return w.pkg;
}
