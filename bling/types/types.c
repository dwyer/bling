#include "bling/ast/ast.h"
#include "bling/emitter/emitter.h"
#include "bling/parser/parser.h"
#include "bling/types/types.h"

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

extern char *types_declString(decl_t *decl) {
    emitter_t e = {};
    print_decl(&e, decl);
    return emitter_string(&e);
}

extern char *types_exprString(expr_t *expr) {
    emitter_t e = {};
    print_expr(&e, expr);
    return emitter_string(&e);
}

extern char *types_stmtString(stmt_t *stmt) {
    emitter_t e = {};
    print_stmt(&e, stmt);
    return emitter_string(&e);
}

extern char *types_typeString(expr_t *expr) {
    emitter_t e = {};
    print_type(&e, expr);
    return emitter_string(&e);
}

extern bool types_isType(expr_t *expr) {
    switch (expr->type) {
    case ast_EXPR_IDENT:
        if (expr->ident.obj == NULL) {
            panic("types_isType: unresolved identifier %s", expr->ident.name);
        }
        return expr->ident.obj->decl->type == ast_DECL_TYPEDEF;
    case ast_EXPR_STAR:
        return types_isType(expr->star.x);
    case ast_TYPE_ARRAY:
    case ast_TYPE_ENUM:
    case ast_TYPE_FUNC:
    case ast_TYPE_NATIVE:
    case ast_TYPE_STRUCT:
        return true;
    default:
        return false;
    }
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
        .pos = type->pos,
        .star = {
            .x = type,
        }
    };
    return esc(x);
}

static expr_t *lookup_typedef(expr_t *ident) {
    decl_t *decl = ident->ident.obj->decl;
    if (decl->type != ast_DECL_TYPEDEF) {
        panic("not a typedef: %s", types_typeString(ident));
    }
    return decl->typedef_.type;
}

static expr_t *types_getBaseType(expr_t *type) {
    for (;;) {
        switch (type->type) {
        case ast_EXPR_IDENT:
            type = lookup_typedef(type);
            break;
        case ast_TYPE_ARRAY:
        case ast_TYPE_ENUM:
        case ast_TYPE_NATIVE:
        case ast_TYPE_STRUCT:
            return type;
        default:
            panic("not a typedef: %s", types_typeString(type));
        }
    }
}

static bool types_isArithmetic(expr_t *type) {
    switch (type->type) {
    case ast_EXPR_IDENT:
        return types_isArithmetic(types_getBaseType(type));
    case ast_EXPR_STAR:
    case ast_TYPE_ENUM:
        return true;
    case ast_TYPE_NATIVE:
        return !streq(type->native.name, "void");
    default:
        return false;
    }
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

static bool types_areIdentical(expr_t *a, expr_t *b) {
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
    case ast_EXPR_IDENT:
        return b->type == ast_EXPR_IDENT && a->ident.obj == a->ident.obj;
    case ast_EXPR_STAR:
        if (ast_isVoidPtr(a) || ast_isVoidPtr(b)) {
            return true;
        }
        return types_areIdentical(a->star.x, b->star.x);
    case ast_TYPE_ARRAY:
        // TODO check lengths
        return types_areIdentical(a->array.elt, b->array.elt);
    case ast_TYPE_FUNC:
        {
            if (!types_areIdentical(a->func.result, b->func.result)) {
                return false;
            }
            int i = 0;
            for (; a->func.params[i]; i++) {
                decl_t *param1 = a->func.params[i];
                decl_t *param2 = b->func.params[i];
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
        return true;
    default:
        panic("unreachable: %s == %s", types_typeString(a), types_typeString(b));
        return false;
    }
}

static bool types_isPointer(expr_t *t) {
    return t->type == ast_EXPR_STAR || t->type == ast_TYPE_ARRAY;
}

static expr_t *types_pointerBase(expr_t *t) {
    switch (t->type) {
    case ast_EXPR_STAR:
        return t->star.x;
    case ast_TYPE_ARRAY:
        return t->array.elt;
    default:
        panic("not a pointer: %s", types_typeString(t));
        return NULL;
    }
}

static bool types_areAssignable(expr_t *a, expr_t *b) {
    if (types_areIdentical(a, b)) {
        return true;
    }
    if (ast_isVoidPtr(a) || ast_isVoidPtr(b)) {
        return true;
    }
    if (types_isPointer(a) && types_isPointer(b)) {
        return types_areAssignable(types_pointerBase(a), types_pointerBase(b));
    }
    while (a->type == ast_EXPR_IDENT) {
        a = lookup_typedef(a);
    }
    while (b->type == ast_EXPR_IDENT) {
        b = lookup_typedef(b);
    }
    if (types_isNative(a, "bool") && types_isArithmetic(b)) {
        return true;
    }
    if (b->type == ast_TYPE_ENUM && types_isArithmetic(a)) {
        return true;
    }
    if (a->type == ast_TYPE_ENUM && types_isArithmetic(a)) {
        return true;
    }
    return types_areIdentical(a, b);
}

static bool types_areComparable(expr_t *a, expr_t *b) {
    if (types_areIdentical(a, b)) {
        return true;
    }
    if (types_isArithmetic(a) && types_isArithmetic(b)) {
        return true;
    }
    if (a->type == ast_EXPR_STAR && types_isNative(b, "int")) {
        return true;
    }
    return types_areIdentical(a, b);
}

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
    ast_Object *obj = object_new(kind, ident->ident.name);
    obj->decl = decl;
    ident->ident.obj = obj;
    ast_Object *alt = scope_insert(s, obj);
    if (alt != NULL) {
        if (alt->kind != kind) {
            panic("incompatible redefinition of `%s`: %s",
                    types_declString(obj->decl),
                    types_declString(decl));
        }
        bool redecl = false;
        switch (kind) {
        case obj_kind_FUNC:
            redecl =
                (alt->decl->func.body == NULL || decl->func.body == NULL) &&
                types_areIdentical(alt->decl->func.type, decl->func.type);
            break;
        case obj_kind_TYPE:
            if (alt->decl->typedef_.type->type == ast_TYPE_STRUCT) {
                redecl = alt->decl->typedef_.type->struct_.fields == NULL;
            }
            break;
        case obj_kind_VALUE:
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

extern void declare_builtins(scope_t *s) {
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
        scope_declare(s, esc(decl));
    }
}

typedef struct {
    config_t *conf;
    scope_t *scope;
    expr_t *result;
    slice_t files;
    expr_t *typedefName;
    slice_t filenames;
} checker_t;

static void checker_openScope(checker_t *w) {
    w->scope = scope_new(w->scope);
}

static void checker_closeScope(checker_t *w) {
    scope_t *inner = w->scope;
    w->scope = w->scope->outer;
    scope_free(inner);
}

static expr_t *check_expr(checker_t *w, expr_t *expr);

static void check_type(checker_t *w, expr_t *expr) {
    assert(expr);
    switch (expr->type) {

    case ast_EXPR_IDENT:
        scope_resolve(w->scope, expr);
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
            if (w->typedefName) {
                decl->value.type = w->typedefName;
            }
            scope_declare(w->scope, decl);
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

    case ast_TYPE_STRUCT:
        if (expr->struct_.fields) {
            checker_openScope(w);
            for (int i = 0; expr->struct_.fields[i]; i++) {
                decl_t *field = expr->struct_.fields[i];
                check_type(w, field->field.type);
                if (field->field.name) {
                    scope_declare(w->scope, field);
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
    ast_Object *obj = ident->ident.obj;
    assert(obj);
    return get_decl_type(obj->decl);
}

static decl_t *find_field(expr_t *type, expr_t *sel) {
    assert(type->type == ast_TYPE_STRUCT);
    assert(sel->type == ast_EXPR_IDENT);
    for (int i = 0; type->struct_.fields && type->struct_.fields[i]; i++) {
        decl_t *field = type->struct_.fields[i];
        expr_t *name = field->field.name;
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

static bool types_isInteger(expr_t *x) {
    switch (x->type) {
    case ast_EXPR_IDENT:
        return types_isInteger(x->ident.obj->decl->typedef_.type);
    case ast_TYPE_ENUM:
    case ast_TYPE_NATIVE:
        return true;
    default:
        return false;
    }
}

static void check_array(checker_t *w, expr_t *x) {
    expr_t *baseT = types_getBaseType(x->compound.type);
    for (int i = 0; x->compound.list[i]; i++) {
        expr_t *elt = x->compound.list[i];
        if (elt->type == ast_EXPR_KEY_VALUE) {
            elt->key_value.isArray = true;
            expr_t *indexT = check_expr(w, elt->key_value.key);
            if (!types_isInteger(indexT)) {
                panic("not a valid index: %s: %s",
                        types_exprString(elt->key_value.key),
                        types_exprString(elt));
            }
            elt = elt->key_value.value;
        }
        if (elt->type == ast_EXPR_COMPOUND) {
            if (elt->compound.type == NULL) {
                elt->compound.type = baseT->array.elt;
            }
        }
        expr_t *eltT = check_expr(w, elt);
        (void)eltT;
    }
}

static decl_t *getStructField(expr_t *type, int index) {
    assert(type->type == ast_TYPE_STRUCT);
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

static void check_struct(checker_t *w, expr_t *x) {
    assert(x->compound.type);
    expr_t *baseT = types_getBaseType(x->compound.type);
    bool expectKV = false;
    for (int i = 0; x->compound.list[i]; i++) {
        expr_t *elt = x->compound.list[i];
        expr_t *fieldT = NULL;
        if (elt->type == ast_EXPR_KEY_VALUE) {
            elt->key_value.isArray = false;
            expectKV = true;
            expr_t *key = elt->key_value.key;
            if (!ast_isIdent(key)) {
                panic("key must be an identifier: %s",
                        types_typeString(x->compound.type));
            }
            decl_t *field = find_field(baseT, key);
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
            decl_t *field = getStructField(baseT, i);
            fieldT = field->field.type;
        }
        if (elt->type == ast_EXPR_COMPOUND) {
            if (elt->compound.type == NULL) {
                elt->compound.type = fieldT;
            }
        }
        expr_t *eltT = check_expr(w, elt);
        if (!types_areAssignable(fieldT, eltT)) {
            panic("cannot init field of type `%s` with value of type `%s`: %s",
                    types_typeString(fieldT),
                    types_typeString(eltT),
                    types_exprString(elt));
        }
    }
}

static void check_compositeLit(checker_t *w, expr_t *x) {
    expr_t *t = x->compound.type;
    assert(t);
    expr_t *baseT = types_getBaseType(t);
    switch (baseT->type) {
    case ast_TYPE_ARRAY:
        check_array(w, x);
        break;
    case ast_TYPE_STRUCT:
        check_struct(w, x);
        break;
    default:
        panic("composite type must be an array or a struct: %s",
                types_typeString(t));
    }
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
            case token_LAND:
            case token_LOR:
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
            case token_CHAR:
                type = types_makeIdent("char");
                break;
            case token_FLOAT:
                type = types_makeIdent("float");
                break;
            case token_INT:
                type = types_makeIdent("int");
                break;
            case token_STRING:
                type = types_makePtr(types_makeIdent("char"));
                break;
            default:
                panic("check_expr: not implmented: %s", token_string(kind));
                break;
            }
            check_type(w, type);
            return type;
        }

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

    case ast_EXPR_COMPOUND:
        if (expr->compound.type == NULL) {
            panic("untyped compound expr: %s", types_exprString(expr));
        }
        check_compositeLit(w, expr);
        return expr->compound.type;

    case ast_EXPR_CAST:
        {
            check_type(w, expr->cast.type);
            if (expr->cast.expr->type == ast_EXPR_COMPOUND) {
                expr->cast.expr->compound.type = expr->cast.type;
            }
            check_expr(w, expr->cast.expr);
            return expr->cast.type;
        }

    case ast_EXPR_IDENT:
        {
            scope_resolve(w->scope, expr);
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
            if (type->type == ast_EXPR_STAR) {
                expr->selector.tok = token_ARROW;
                type = type->star.x;
            } else {
                assert(expr->selector.tok != token_ARROW);
            }
            type = types_getBaseType(type);
            decl_t *field = find_field(type, expr->selector.sel);
            if (field == NULL) {
                panic("struct `%s` (`%s`) has no field `%s`",
                        types_exprString(expr->selector.x),
                        types_typeString(type),
                        expr->selector.sel->ident.name);
            }
            expr->selector.sel->ident.obj = field->field.name->ident.obj;
            return field->field.type;
        }

    case ast_EXPR_SIZEOF:
        {
            check_type(w, expr->sizeof_.x);
            expr_t *ident = types_makeIdent("size_t");
            scope_resolve(w->scope, ident);
            return ident;
        }

    case ast_EXPR_STAR:
        {
            expr_t *type = check_expr(w, expr->star.x);
            switch (type->type) {
            case ast_EXPR_STAR:
                return type->star.x;
            case ast_TYPE_ARRAY:
                return type->array.elt;
            default:
                panic("check_expr: derefencing a non-pointer `%s`: %s",
                        types_typeString(type),
                        types_exprString(expr));
                return NULL;
            }
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
            if (a->is_const) {
                panic("cannot assign to const var: %s", types_stmtString(stmt));
            }
            expr_t *b = check_expr(w, stmt->assign.y);
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
        if (stmt->if_.else_) {
            check_stmt(w, stmt->if_.else_);
        }
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
                for (int j = 0; clause->case_.exprs && clause->case_.exprs[j]; j++) {
                    expr_t *type2 = check_expr(w, clause->case_.exprs[j]);
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

static char *constant_stringVal(expr_t *x) {
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

static void check_file(checker_t *w, ast_File *file);

static void check_import(checker_t *w, decl_t *imp) {
    char *path = constant_stringVal(imp->imp.path);
    for (int i = 0; i < len(w->filenames); i++) {
        char *s = NULL;
        slice_get(&w->filenames, i, &s);
        if (streq(path, s)) {
            free(path);
            return;
        }
    }
    w->filenames = append(w->filenames, &path);
    error_t *err = NULL;
    ast_File **files = parser_parseDir(path, &err);
    if (err) {
        panic("%s: %s", path, err->error);
    }
    for (int i = 0; files[i]; i++) {
        check_file(w, files[i]);
    }
}

static void check_decl(checker_t *w, decl_t *decl) {
    switch (decl->type) {
    case ast_DECL_FUNC:
        check_type(w, decl->func.type);
        scope_declare(w->scope, decl);
        break;
    case ast_DECL_PRAGMA:
        break;
    case ast_DECL_TYPEDEF:
        w->typedefName = decl->typedef_.name;
        check_type(w, decl->typedef_.type);
        w->typedefName = NULL;
        scope_declare(w->scope, decl);
        break;
    case ast_DECL_VALUE:
        {
            expr_t *valType = NULL;
            if (decl->value.type != NULL) {
                check_type(w, decl->value.type);
            }
            if (decl->value.value != NULL) {
                if (decl->value.value->type == ast_EXPR_COMPOUND) {
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
                expr_t *varType = decl->value.type;
                if (!types_areAssignable(varType, valType)) {
                    panic("check_decl: not assignable %s and %s: %s",
                            types_typeString(varType),
                            types_typeString(valType),
                            types_declString(decl));
                }
            }
            scope_declare(w->scope, decl);
            break;
        }
    default:
        panic("check_decl: not implemented: %s", types_declString(decl));
    }
}

static void check_func(checker_t *w, decl_t *decl) {
    if (decl->type == ast_DECL_FUNC && decl->func.body) {
        checker_openScope(w);
        expr_t *type = decl->func.type;
        for (int i = 0; type->func.params && type->func.params[i]; i++) {
            decl_t *param = type->func.params[i];
            assert(param->type == ast_DECL_FIELD);
            if (param->field.name) {
                scope_declare(w->scope, param);
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

static void check_file(checker_t *w, ast_File *file) {
    for (int i = 0; file->imports[i] != NULL; i++) {
        check_import(w, file->imports[i]);
    }
    w->files = append(w->files, &file);
    if (w->conf->strict) {
        for (int i = 0; file->decls[i] != NULL; i++) {
            check_decl(w, file->decls[i]);
        }
        for (int i = 0; file->decls[i] != NULL; i++) {
            check_func(w, file->decls[i]);
        }
    }
}

extern package_t types_checkFile(config_t *conf, ast_File *file) {
    checker_t w = {
        .conf = conf,
        .scope = file->scope,
        .files = slice_init(sizeof(ast_File *)),
        .filenames = slice_init(sizeof(char *)),
    };
    check_file(&w, file);
    package_t pkg = {
        .files = slice_to_nil_array(w.files),
        .scope = w.scope,
    };
    return pkg;
}
