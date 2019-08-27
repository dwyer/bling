#include "bling/types/types.h"

#include "bling/ast/ast.h"
#include "bling/emitter/emitter.h"
#include "bling/parser/parser.h"
#include "sys/sys.h"

extern char *types$constant_stringVal(ast$Expr *x) {
    // TODO move this to const pkg
    assert(x->kind == ast$EXPR_BASIC_LIT);
    const char *lit = x->basic.value;
    int n = sys$strlen(lit) - 2;
    char *val = sys$malloc(n + 1);
    for (int i = 0; i < n; i++) {
        val[i] = lit[i+1];
    }
    val[n] = '\0';
    return val;
}

extern char *types$declString(ast$Decl *decl) {
    emitter$Emitter e = {};
    emitter$emitDecl(&e, decl);
    return emitter$Emitter_string(&e);
}

extern char *types$exprString(ast$Expr *expr) {
    emitter$Emitter e = {};
    emitter$emitExpr(&e, expr);
    return emitter$Emitter_string(&e);
}

extern char *types$stmtString(ast$Stmt *stmt) {
    emitter$Emitter e = {};
    emitter$emitStmt(&e, stmt);
    return emitter$Emitter_string(&e);
}

extern char *types$typeString(ast$Expr *expr) {
    emitter$Emitter e = {};
    emitter$emitType(&e, expr);
    return emitter$Emitter_string(&e);
}

extern bool types$isType(ast$Expr *expr) {
    switch (expr->kind) {
    case ast$EXPR_IDENT:
        if (expr->ident.obj == NULL) {
            panic(sys$sprintf("types$isType: unresolved identifier %s",
                        expr->ident.name));
        }
        return expr->ident.obj->decl->kind == ast$DECL_TYPEDEF;
    case ast$EXPR_STAR:
        return types$isType(expr->star.x);
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

static ast$Expr *types$makeIdent(const char *name) {
    ast$Expr x = {
        .kind = ast$EXPR_IDENT,
        .ident = {
            .name = sys$strdup(name),
        },
    };
    return esc(x);
}

static ast$Expr *types$makePtr(ast$Expr *type) {
    ast$Expr x = {
        .kind = ast$EXPR_STAR,
        .star = {
            .x = type,
        }
    };
    return esc(x);
}

static ast$Expr *getUnderlyingType(ast$Expr *ident) {
    if (ident->ident.obj == NULL) {
        panic(sys$sprintf("not resolved: %s", types$typeString(ident)));
    }
    ast$Decl *decl = ident->ident.obj->decl;
    if (decl->kind != ast$DECL_TYPEDEF) {
        panic(sys$sprintf("not a type: %s", types$typeString(ident)));
    }
    return decl->typedef_.type;
}

static ast$Expr *types$getBaseType(ast$Expr *type) {
    for (;;) {
        switch (type->kind) {
        case ast$EXPR_IDENT:
            type = getUnderlyingType(type);
            break;
        case ast$EXPR_SELECTOR:
            type = type->selector.sel;
            break;
        case ast$TYPE_ARRAY:
        case ast$TYPE_ENUM:
        case ast$TYPE_MAP:
        case ast$TYPE_NATIVE:
        case ast$TYPE_STRUCT:
            return type;
        default:
            panic(sys$sprintf("not a type: %s", types$typeString(type)));
        }
    }
}

static bool types$isArithmetic(ast$Expr *type) {
    switch (type->kind) {
    case ast$EXPR_IDENT:
        return types$isArithmetic(types$getBaseType(type));
    case ast$EXPR_STAR:
    case ast$TYPE_ENUM:
        return true;
    case ast$TYPE_NATIVE:
        return !sys$streq(type->native.name, "void");
    default:
        return false;
    }
}

static bool types$isNative(ast$Expr *type, const char *name) {
    switch (type->kind) {
    case ast$EXPR_IDENT:
        return sys$streq(type->ident.name, name);
    case ast$TYPE_NATIVE:
        return true;
    default:
        return false;
    }
}

static bool types$areIdentical(ast$Expr *a, ast$Expr *b) {
    if (a == b) {
        return true;
    }
    if (a == NULL || b == NULL) {
        return false;
    }
    if (a->kind == ast$TYPE_MAP || b->kind == ast$TYPE_MAP) {
        return true;
    }
    assert(a);
    assert(b);
    if (a->kind == ast$EXPR_SELECTOR) {
        a = a->selector.sel;
    }
    if (b->kind == ast$EXPR_SELECTOR) {
        b = b->selector.sel;
    }
    if (a->kind != b->kind) {
        return false;
    }
    switch (a->kind) {
    case ast$EXPR_IDENT:
        return b->kind == ast$EXPR_IDENT && a->ident.obj == a->ident.obj;
    case ast$EXPR_STAR:
        return types$areIdentical(a->star.x, b->star.x);
    case ast$TYPE_ARRAY:
        // TODO check lengths
        return types$areIdentical(a->array.elt, b->array.elt);
    case ast$TYPE_FUNC:
        {
            if (!types$areIdentical(a->func.result, b->func.result)) {
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
                    if (!types$areIdentical(param1->field.type, param2->field.type)) {
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
        panic(sys$sprintf("not implemented: %s == %s",
                    types$typeString(a), types$typeString(b)));
        return false;
    }
}

static bool types$isPointer(ast$Expr *t) {
    return t->kind == ast$EXPR_STAR || t->kind == ast$TYPE_ARRAY;
}

static ast$Expr *types$pointerBase(ast$Expr *t) {
    switch (t->kind) {
    case ast$EXPR_STAR:
        return t->star.x;
    case ast$TYPE_ARRAY:
        return t->array.elt;
    default:
        panic(sys$sprintf("not a pointer: %s", types$typeString(t)));
        return NULL;
    }
}

static bool types$areAssignable(ast$Expr *a, ast$Expr *b) {
    if (a->kind == ast$TYPE_ELLIPSIS) {
        return true;
    }
    if (types$areIdentical(a, b)) {
        return true;
    }
    if (ast$isVoidPtr(a) || ast$isVoidPtr(b)) {
        return true;
    }
    if (types$isPointer(a) && types$isPointer(b)) {
        return types$areAssignable(types$pointerBase(a), types$pointerBase(b));
    }
    while (a->kind == ast$EXPR_IDENT) {
        a = getUnderlyingType(a);
    }
    while (b->kind == ast$EXPR_IDENT) {
        b = getUnderlyingType(b);
    }
    if (types$isNative(a, "bool") && types$isArithmetic(b)) {
        return true;
    }
    if (b->kind == ast$TYPE_ENUM && types$isArithmetic(a)) {
        return true;
    }
    if (a->kind == ast$TYPE_ENUM && types$isArithmetic(a)) {
        return true;
    }
    return types$areIdentical(a, b);
}

static bool types$areComparable(ast$Expr *a, ast$Expr *b) {
    if (types$areIdentical(a, b)) {
        return true;
    }
    if (types$isArithmetic(a) && types$isArithmetic(b)) {
        return true;
    }
    if (a->kind == ast$EXPR_STAR && types$isNative(b, "int")) {
        return true;
    }
    return types$areIdentical(a, b);
}

static bool types$isLhs(ast$Expr *expr) {
    switch (expr->kind) {
    case ast$EXPR_IDENT:
    case ast$EXPR_SELECTOR:
        return true;
    case ast$EXPR_CAST:
        return types$isLhs(expr->cast.expr);
    case ast$EXPR_INDEX:
        return types$isLhs(expr->index.x);
    case ast$EXPR_PAREN:
        return types$isLhs(expr->paren.x);
    case ast$EXPR_STAR:
        return types$isLhs(expr->star.x);
    default:
        return false;
    }
}

static ast$Expr *getDeclType(ast$Decl *decl) {
    assert(decl);
    switch (decl->kind) {
    case ast$DECL_FIELD:
        return decl->field.type;
    case ast$DECL_FUNC:
        return decl->func.type;
    case ast$DECL_IMPORT:
        return NULL;
    case ast$DECL_TYPEDEF:
        return decl->typedef_.type;
    case ast$DECL_VALUE:
        return decl->value.type;
    default:
        panic(sys$sprintf("unhandled decl: %s", types$declString(decl)));
        return NULL;
    }
}

static ast$Decl *getStructFieldByName(ast$Expr *type, ast$Expr *name) {
    ast$Expr *base = types$getBaseType(type);
    assert(base->kind == ast$TYPE_STRUCT);
    assert(name->kind == ast$EXPR_IDENT);
    for (int i = 0; base->struct_.fields && base->struct_.fields[i]; i++) {
        ast$Decl *field = base->struct_.fields[i];
        if (field->field.name) {
            if (sys$streq(name->ident.name, field->field.name->ident.name)) {
                base = field->field.type;
                return field;
            }
        } else {
            ast$Decl *subField = getStructFieldByName(field->field.type, name);
            if (subField) {
                return subField;
            }
        }
    }
    return NULL;
}

static ast$Decl *getStructField(ast$Expr *type, int index) {
    ast$Expr *base = types$getBaseType(type);
    if (base->kind != ast$TYPE_STRUCT) {
        panic(sys$sprintf("not a struct: %s", types$typeString(type)));
    }
    if (base->struct_.fields == NULL) {
        panic(sys$sprintf("incomplete field defn: %s", types$typeString(type)));
    }
    for (int i = 0; base->struct_.fields[i]; i++) {
        if (i == index) {
            return base->struct_.fields[i];
        }
    }
    return NULL;
}

typedef struct {
    types$Config *conf;
    token$FileSet *fset;
    types$Package *pkg;
    types$Info *info;

    ast$Expr *result;
} Checker;

static void Checker_error(Checker *c, token$Pos pos, const char *msg) {
    token$FileSet_print(c->fset);
    token$Position position = {};
    token$File *file = token$FileSet_file(c->fset, pos);
    if (file) {
        position = token$File_position(file, pos);
    }
    panic(sys$sprintf("%s: %s\n%s",
                token$Position_string(&position),
                msg,
                token$File_lineString(file, position.line)));
}

static void Checker_resolve(Checker *c, ast$Scope *s, ast$Expr *x) {
    assert(x->kind == ast$EXPR_IDENT);
    if (x->ident.obj) {
        Checker_error(c, ast$Expr_pos(x), "already resolved");
    }
    if (ast$resolve(s, x)) {
        return;
    }
    Checker_error(c, ast$Expr_pos(x),
            sys$sprintf("unresolved: %s", x->ident.name));
}

static void Checker_declare(Checker *c, ast$Decl *decl, void *data,
        ast$Scope *scope, ast$ObjKind kind, ast$Expr *ident) {
    if (ident->ident.obj != NULL) {
        Checker_error(c, decl->pos,
                sys$sprintf("already declared: %s", ident->ident.name));
    }
    ast$Object *obj = ast$newObject(kind, ident->ident.name);
    obj->decl = decl;
    obj->data = data;
    obj->scope = scope;
    ident->ident.obj = obj;
    ast$Object *alt = ast$Scope_insert(scope, obj);
    if (alt != NULL) {
        Checker_error(c, decl->pos,
                sys$sprintf("incompatible redefinition of `%s`: %s",
                    types$declString(obj->decl),
                    types$declString(decl)));
    }
}

static void Checker_openScope(Checker *c) {
    c->pkg->scope = ast$Scope_new(c->pkg->scope);
}

static void Checker_closeScope(Checker *c) {
    // ast$Scope *inner = c->pkg->scope;
    c->pkg->scope = c->pkg->scope->outer;
    // ast$Scope_free(inner);
}

static ast$Expr *Checker_checkExpr(Checker *c, ast$Expr *expr);

static void Checker_checkType(Checker *c, ast$Expr *t) {
    assert(t);
    switch (t->kind) {

    case ast$EXPR_IDENT:
        Checker_resolve(c, c->pkg->scope, t);
        break;

    case ast$EXPR_SELECTOR:
        {
            ast$Expr *type = Checker_checkExpr(c, t->selector.x);
            assert(type == NULL);
            assert(ast$isIdent(t->selector.x));
            assert(t->selector.x->ident.obj->kind == ast$ObjKind_PKG);
            ast$Scope *oldScope = c->pkg->scope;
            c->pkg->scope = t->selector.x->ident.obj->data;
            Checker_checkType(c, t->selector.sel);
            c->pkg->scope = oldScope;
        }
        break;

    case ast$EXPR_STAR:
        Checker_checkType(c, t->star.x);
        break;

    case ast$TYPE_ARRAY:
        Checker_checkType(c, t->array.elt);
        if (t->array.len) {
            ast$Expr *len = Checker_checkExpr(c, t->array.len);
            (void)len; // TODO assert that len resolves to int
        }
        break;

    case ast$TYPE_ELLIPSIS:
        break;

    case ast$TYPE_ENUM:
        for (int i = 0; t->enum_.enums[i]; i++) {
            ast$Decl *decl = t->enum_.enums[i];
            if (t->enum_.name) {
                decl->value.type = t->enum_.name;
            }
            Checker_declare(c, decl, NULL, c->pkg->scope, ast$ObjKind_CON,
                    decl->value.name);
            if (decl->value.value) {
                Checker_checkExpr(c, decl->value.value);
            }
        }
        break;

    case ast$TYPE_FUNC:
        for (int i = 0; t->func.params && t->func.params[i]; i++) {
            ast$Decl *param = t->func.params[i];
            assert(param->kind == ast$DECL_FIELD);
            Checker_checkType(c, param->field.type);
        }
        if (t->func.result) {
            Checker_checkType(c, t->func.result);
        }
        break;

    case ast$TYPE_MAP:
        if (t->map_.val) {
            Checker_checkType(c, t->map_.val);
        }
        break;

    case ast$TYPE_STRUCT:
        if (t->struct_.fields) {
            Checker_openScope(c);
            for (int i = 0; t->struct_.fields[i]; i++) {
                ast$Decl *field = t->struct_.fields[i];
                Checker_checkType(c, field->field.type);
                if (field->field.name) {
                    Checker_declare(c, field, NULL, c->pkg->scope, ast$ObjKind_VAL,
                            field->field.name);
                }
            }
            Checker_closeScope(c);
        }
        break;

    default:
        Checker_error(c, ast$Expr_pos(t), "unknown type");
        break;
    }
}

static ast$Expr *Checker_checkIdent(Checker *c, ast$Expr *expr) {
    assert(expr->kind == ast$EXPR_IDENT);
    if (expr->ident.obj == NULL) {
        Checker_error(c, ast$Expr_pos(expr), "unresolved identifier");
    }
    return getDeclType(expr->ident.obj->decl);
}

static bool types$isInteger(ast$Expr *x) {
    switch (x->kind) {
    case ast$EXPR_IDENT:
        return types$isInteger(x->ident.obj->decl->typedef_.type);
    case ast$TYPE_ENUM:
    case ast$TYPE_NATIVE:
        return true;
    default:
        return false;
    }
}

static ast$Expr *Checker_checkCompositeLit(Checker *c, ast$Expr *x);

static void Checker_checkArrayLit(Checker *c, ast$Expr *x) {
    ast$Expr *baseT = types$getBaseType(x->composite.type);
    for (int i = 0; x->composite.list[i]; i++) {
        ast$Expr *elt = x->composite.list[i];
        if (elt->kind == ast$EXPR_KEY_VALUE) {
            elt->key_value.isArray = true;
            ast$Expr *indexT = Checker_checkExpr(c, elt->key_value.key);
            if (!types$isInteger(indexT)) {
                Checker_error(c, ast$Expr_pos(elt),
                        sys$sprintf("not a valid index: %s",
                            types$exprString(elt->key_value.key)));
            }
            elt = elt->key_value.value;
        }
        if (elt->kind == ast$EXPR_COMPOSITE_LIT) {
            if (elt->composite.type == NULL) {
                elt->composite.type = baseT->array.elt;
            } else {
                Checker_checkType(c, elt->composite.type);
            }
            Checker_checkCompositeLit(c, elt);
        } else {
            Checker_checkExpr(c, elt);
        }
    }
}

static void Checker_checkStructLit(Checker *c, ast$Expr *x) {
    assert(x->composite.type);
    bool expectKV = false;
    for (int i = 0; x->composite.list[i]; i++) {
        ast$Expr *elt = x->composite.list[i];
        ast$Expr *fieldT = NULL;
        if (elt->kind == ast$EXPR_KEY_VALUE) {
            elt->key_value.isArray = false;
            expectKV = true;
            ast$Expr *key = elt->key_value.key;
            if (!ast$isIdent(key)) {
                Checker_error(c, ast$Expr_pos(x), "key must be an identifier");
            }
            ast$Decl *field = getStructFieldByName(x->composite.type, key);
            if (field == NULL) {
                Checker_error(c, ast$Expr_pos(key),
                        sys$sprintf("no member named '%s' in '%s'",
                            types$exprString(key),
                            types$typeString(x->composite.type)));
            }
            key->ident.obj = field->field.name->ident.obj;
            fieldT = field->field.type;
            elt = elt->key_value.value;
        } else {
            if (expectKV) {
                Checker_error(c, ast$Expr_pos(x), "expected a key/value expr");
            }
            ast$Decl *field = getStructField(x->composite.type, i);
            fieldT = field->field.type;
        }
        ast$Expr *eltT = NULL;
        if (elt->kind == ast$EXPR_COMPOSITE_LIT) {
            if (elt->composite.type == NULL) {
                elt->composite.type = fieldT;
            } else {
                Checker_checkType(c, elt->composite.type);
            }
            eltT = Checker_checkCompositeLit(c, elt);
        } else {
            eltT = Checker_checkExpr(c, elt);
        }
        if (!types$areAssignable(fieldT, eltT)) {
            Checker_error(c, ast$Expr_pos(elt), sys$sprintf(
                        "cannot init field of type `%s` with value of type `%s`",
                        types$typeString(fieldT),
                        types$typeString(eltT)));
        }
    }
}

static ast$Expr *Checker_checkCompositeLit(Checker *c, ast$Expr *x) {
    ast$Expr *t = x->composite.type;
    assert(t);
    ast$Expr *baseT = types$getBaseType(t);
    switch (baseT->kind) {
    case ast$TYPE_ARRAY:
        Checker_checkArrayLit(c, x);
        break;
    case ast$TYPE_STRUCT:
        Checker_checkStructLit(c, x);
        break;
    default:
        Checker_error(c, ast$Expr_pos(x),
                "composite type must be an array or a struct");
        break;
    }
    return t;
}

extern ast$Expr *Checker_lookupIdent(Checker *c, char *name) {
    ast$Object *obj = ast$Scope_deepLookup(c->pkg->scope, name);
    assert(obj);
    assert(obj->kind == ast$ObjKind_TYP);
    assert(obj->decl->kind == ast$DECL_TYPEDEF);
    ast$Expr *t = obj->decl->typedef_.name;
    assert(t->ident.obj);
    return t;
}

static ast$Expr *Checker_checkExpr(Checker *c, ast$Expr *expr) {
    assert(expr);
    switch (expr->kind) {

    case ast$EXPR_BINARY:
        {
            ast$Expr *typ1 = Checker_checkExpr(c, expr->binary.x);
            ast$Expr *typ2 = Checker_checkExpr(c, expr->binary.y);
            if (!types$areComparable(typ1, typ2)) {
                Checker_error(c, ast$Expr_pos(expr),
                        sys$sprintf("not compariable: %s and %s",
                            types$typeString(typ1),
                            types$typeString(typ2)));
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
                return Checker_lookupIdent(c, "bool");
            default:
                return typ1;
            }
        }

    case ast$EXPR_BASIC_LIT:
        switch (expr->basic.kind) {
        case token$CHAR:
            return Checker_lookupIdent(c, "char");
        case token$FLOAT:
            return Checker_lookupIdent(c, "float");
        case token$INT:
            return Checker_lookupIdent(c, "int");
        case token$STRING:
            return types$makePtr(Checker_lookupIdent(c, "char"));
        default:
            Checker_error(c, ast$Expr_pos(expr), "unreachable");
            return NULL;
        }

    case ast$EXPR_CALL:
        {
            ast$Expr *func = expr->call.func;
            ast$Expr *type = Checker_checkExpr(c, func);
            if (type->kind == ast$EXPR_STAR) {
                type = type->star.x;
            }
            if (type->kind == ast$TYPE_BUILTIN) {
                int i = 0;
                for (; expr->call.args[i]; i++) {
                    Checker_checkExpr(c, expr->call.args[i]);
                }
                if (type->builtin.variadic) {
                    assert(i == type->builtin.nargs);
                } else {
                    assert(i >= type->builtin.nargs);
                }
                switch (type->builtin.id) {
                case types$LEN:
                    return Checker_lookupIdent(c, "int");
                case types$MAKEMAP:
                    return NULL;
                default:
                    assert(!type->builtin.isExpr);
                    return NULL; // TODO return correct type
                }
            } else if (type->kind == ast$TYPE_FUNC) {
                int j = 0;
                for (int i = 0; expr->call.args[i]; i++) {
                    ast$Decl *param = type->func.params[j];
                    if (param == NULL) {
                        Checker_error(c, ast$Expr_pos(expr), "too many args");
                        break;
                    }
                    ast$Expr *type = Checker_checkExpr(c, expr->call.args[i]);
                    assert(param->kind == ast$DECL_FIELD);
                    if (!types$areAssignable(param->field.type, type)) {
                        Checker_error(c, ast$Expr_pos(expr), sys$sprintf(
                                    "not assignable: %s and %s",
                                    types$typeString(param->field.type),
                                    types$typeString(type)));
                    }
                    if (param->field.type->kind != ast$TYPE_ELLIPSIS) {
                        j++;
                    }
                }
                return type->func.result;
            } else {
                Checker_error(c, ast$Expr_pos(expr),
                        sys$sprintf("`%s` is not a func",
                            types$exprString(expr->call.func)));
            }

        }

    case ast$EXPR_COMPOSITE_LIT:
        Checker_checkType(c, expr->composite.type);
        Checker_checkCompositeLit(c, expr);
        return expr->composite.type;

    case ast$EXPR_CAST:
        Checker_checkType(c, expr->cast.type);
        Checker_checkExpr(c, expr->cast.expr);
        return expr->cast.type;

    case ast$EXPR_IDENT:
        Checker_resolve(c, c->pkg->scope, expr);
        return Checker_checkIdent(c, expr);

    case ast$EXPR_INDEX:
        {
            ast$Expr *type = Checker_checkExpr(c, expr->index.x);
            switch (type->kind) {
            case ast$TYPE_ARRAY:
                type = type->array.elt;
                break;
            case ast$EXPR_STAR:
                type = type->star.x;
                break;
            default:
                Checker_error(c, ast$Expr_pos(expr),
                        sys$sprintf("indexing a non-array or pointer `%s`",
                            types$typeString(type)));
                break;
            }
            ast$Expr *typ2 = Checker_checkExpr(c, expr->index.index);
            (void)typ2; // TODO assert that typ2 is an integer
            return type;
        }

    case ast$EXPR_PAREN:
        return Checker_checkExpr(c, expr->paren.x);

    case ast$EXPR_SELECTOR:
        {
            ast$Expr *type = Checker_checkExpr(c, expr->selector.x);
            if (type == NULL) {
                ast$Expr *x = expr->selector.x;
                assert(x->kind == ast$EXPR_IDENT);
                ast$Decl *decl = x->ident.obj->decl;
                assert(decl->kind == ast$DECL_IMPORT);
                ast$Scope *oldScope = c->pkg->scope;
                c->pkg->scope = x->ident.obj->data;
                type = Checker_checkExpr(c, expr->selector.sel);
                c->pkg->scope = oldScope;
                expr->selector.tok = token$DOLLAR;
                return type;
            }
            if (type->kind == ast$EXPR_STAR) {
                expr->selector.tok = token$ARROW;
                type = type->star.x;
            } else {
                assert(expr->selector.tok != token$ARROW);
            }
            ast$Decl *field = getStructFieldByName(type, expr->selector.sel);
            if (field == NULL) {
                Checker_error(c, ast$Expr_pos(expr),
                        sys$sprintf("no member named '%s' in '%s'",
                            types$exprString(expr->selector.sel),
                            types$typeString(type)));
            }
            expr->selector.sel->ident.obj = field->field.name->ident.obj;
            return field->field.type;
        }

    case ast$EXPR_SIZEOF:
        Checker_checkType(c, expr->sizeof_.x);
        return Checker_lookupIdent(c, "u64");

    case ast$EXPR_STAR:
        {
            ast$Expr *type = Checker_checkExpr(c, expr->star.x);
            switch (type->kind) {
            case ast$EXPR_STAR:
                return type->star.x;
            case ast$TYPE_ARRAY:
                return type->array.elt;
            default:
                Checker_error(c, ast$Expr_pos(expr),
                        sys$sprintf("derefencing a non-pointer `%s`",
                            types$typeString(type)));
                return NULL;
            }
        }

    case ast$EXPR_TERNARY:
        {
            ast$Expr *t1 = Checker_checkExpr(c, expr->ternary.cond);
            assert(types$isArithmetic(t1));
            ast$Expr *t2 = expr->ternary.x
                ? Checker_checkExpr(c, expr->ternary.x)
                : t1;
            ast$Expr *t3 = Checker_checkExpr(c, expr->ternary.y);
            if (!types$areComparable(t2, t3)) {
                Checker_error(c, ast$Expr_pos(expr), "not comparable");
            }
            return t2;
        }

    case ast$EXPR_UNARY:
        {
            ast$Expr *type = Checker_checkExpr(c, expr->unary.x);
            switch (expr->unary.op) {
            case token$AND:
                if (!types$isLhs(expr->unary.x)) {
                    Checker_error(c, ast$Expr_pos(expr),
                            sys$sprintf("invalid lvalue `%s`",
                                types$exprString(expr->unary.x)));
                }
                return types$makePtr(type);
            case token$LAND:
                return types$makePtr(type);
            default:
                break;
            }
            return type;
        }

    default:
        Checker_error(c, ast$Expr_pos(expr), "unknown expr");
        return NULL;
    }
}

static void Checker_checkDecl(Checker *c, ast$Decl *decl);

static void Checker_checkStmt(Checker *c, ast$Stmt *stmt) {
    switch (stmt->kind) {
    case ast$STMT_ASSIGN:
        {
            if (!types$isLhs(stmt->assign.x)) {
                Checker_error(c, ast$Stmt_pos(stmt),
                        sys$sprintf("invalid lvalue `%s`",
                            types$exprString(stmt->assign.x)));
            }
            ast$Expr *a = Checker_checkExpr(c, stmt->assign.x);
            if (a->is_const) {
                Checker_error(c, ast$Stmt_pos(stmt), "cannot assign to const var");
            }
            ast$Expr *b = Checker_checkExpr(c, stmt->assign.y);
            if (!types$areAssignable(a, b)) {
                Checker_error(c, ast$Stmt_pos(stmt),
                        sys$sprintf("not assignable: `%s` and `%s`",
                            types$typeString(a),
                            types$typeString(b)));
            }
        }
        break;

    case ast$STMT_BLOCK:
        Checker_openScope(c);
        for (int i = 0; stmt->block.stmts[i]; i++) {
            Checker_checkStmt(c, stmt->block.stmts[i]);
        }
        Checker_closeScope(c);
        break;

    case ast$STMT_DECL:
        Checker_checkDecl(c, stmt->decl.decl);
        break;

    case ast$STMT_EXPR:
        Checker_checkExpr(c, stmt->expr.x);
        break;

    case ast$STMT_EMPTY:
        break;

    case ast$STMT_IF:
        Checker_checkExpr(c, stmt->if_.cond);
        Checker_checkStmt(c, stmt->if_.body);
        if (stmt->if_.else_) {
            Checker_checkStmt(c, stmt->if_.else_);
        }
        break;

    case ast$STMT_ITER:
        if (stmt->iter.init || stmt->iter.post) {
            Checker_openScope(c);
        }
        if (stmt->iter.init) {
            Checker_checkStmt(c, stmt->iter.init);
        }
        if (stmt->iter.cond) {
            Checker_checkExpr(c, stmt->iter.cond);
        }
        if (stmt->iter.post) {
            Checker_checkStmt(c, stmt->iter.post);
        }
        Checker_checkStmt(c, stmt->iter.body);
        if (stmt->iter.init || stmt->iter.post) {
            Checker_closeScope(c);
        }
        break;

    case ast$STMT_JUMP:
        /* TODO walk label in label scope */
        break;

    case ast$STMT_LABEL:
        /* TODO walk label in label scope */
        Checker_checkStmt(c, stmt->label.stmt);
        break;

    case ast$STMT_POSTFIX:
        Checker_checkExpr(c, stmt->postfix.x);
        break;

    case ast$STMT_RETURN:
        if (stmt->return_.x) {
            ast$Expr *a = c->result;
            ast$Expr *b = Checker_checkExpr(c, stmt->return_.x);
            if (a == NULL) {
                Checker_error(c, ast$Stmt_pos(stmt), "returning value in void function");
            }
            if (!types$areAssignable(a, b)) {
                Checker_error(c, ast$Stmt_pos(stmt),
                        sys$sprintf("not returnable: %s and %s",
                            types$typeString(a),
                            types$typeString(b)));
            }
        }
        break;

    case ast$STMT_SWITCH:
        {
            ast$Expr *type1 = Checker_checkExpr(c, stmt->switch_.tag);
            for (int i = 0; stmt->switch_.stmts[i]; i++) {
                ast$Stmt *clause = stmt->switch_.stmts[i];
                assert(clause->kind == ast$STMT_CASE);
                for (int j = 0; clause->case_.exprs && clause->case_.exprs[j]; j++) {
                    ast$Expr *type2 = Checker_checkExpr(c, clause->case_.exprs[j]);
                    if (!types$areComparable(type1, type2)) {
                        Checker_error(c, ast$Stmt_pos(stmt),
                                sys$sprintf("not comparable: %s and %s",
                                    types$typeString(type1),
                                    types$typeString(type2)));
                    }
                }
                for (int j = 0; clause->case_.stmts[j]; j++) {
                    Checker_checkStmt(c, clause->case_.stmts[j]);
                }
            }
        }
        break;

    default:
        Checker_error(c, ast$Stmt_pos(stmt), "unknown stmt");
    }
}

static types$Package *Checker_checkImport(Checker *c, ast$Decl *imp) {
    char *path = types$constant_stringVal(imp->imp.path);
    types$Package *pkg = NULL;
    utils$Map_get(&c->info->imports, path, &pkg);
    if (pkg == NULL) {
        pkg = types$check(c->conf, path, c->fset, NULL, c->info);
        assert(utils$Map_get(&c->info->imports, path, NULL));
    }
    imp->imp.name = types$makeIdent(pkg->scope->pkg);
    Checker_declare(c, imp, pkg->scope, c->pkg->scope, ast$ObjKind_PKG,
            imp->imp.name);
    return pkg;
}

static void Checker_checkDecl(Checker *c, ast$Decl *decl) {
    switch (decl->kind) {
    case ast$DECL_FUNC:
        {
            Checker_checkType(c, decl->func.type);
            if (c->conf->ignoreFuncBodies || !decl->func.body) {
                break;
            }
            Checker_openScope(c);
            ast$Expr *type = decl->func.type;
            for (int i = 0; type->func.params && type->func.params[i]; i++) {
                ast$Decl *param = type->func.params[i];
                assert(param->kind == ast$DECL_FIELD);
                if (param->field.name) {
                    Checker_declare(c, param, NULL, c->pkg->scope,
                            ast$ObjKind_VAL, param->field.name);
                }
            }
            c->result = decl->func.type->func.result;
            // walk the block manually to avoid opening a new scope
            for (int i = 0; decl->func.body->block.stmts[i]; i++) {
                Checker_checkStmt(c, decl->func.body->block.stmts[i]);
            }
            c->result = NULL;
            Checker_closeScope(c);
            break;
        }
    case ast$DECL_PRAGMA:
        break;
    case ast$DECL_TYPEDEF:
        switch (decl->typedef_.type->kind) {
        case ast$TYPE_ENUM:
            decl->typedef_.type->enum_.name = decl->typedef_.name;
            break;
        case ast$TYPE_STRUCT:
            decl->typedef_.type->struct_.name = decl->typedef_.name;
            break;
        default:
            break;
        }
        Checker_checkType(c, decl->typedef_.type);
        break;
    case ast$DECL_VALUE:
        {
            ast$Expr *valType = NULL;
            if (decl->value.type != NULL) {
                Checker_checkType(c, decl->value.type);
            }
            if (decl->value.value != NULL) {
                if (decl->value.value->kind == ast$EXPR_COMPOSITE_LIT) {
                    if (decl->value.value->composite.type == NULL) {
                        decl->value.value->composite.type = decl->value.type;
                    } else {
                        Checker_checkType(c, decl->value.value->composite.type);
                    }
                    valType = Checker_checkCompositeLit(c, decl->value.value);
                } else {
                    valType = Checker_checkExpr(c, decl->value.value);
                }
            }
            if (decl->value.type == NULL) {
                decl->value.type = valType;
            }
            if (valType != NULL) {
                ast$Expr *varType = decl->value.type;
                if (!types$areAssignable(varType, valType)) {
                    Checker_error(c, decl->pos,
                            sys$sprintf("not assignable %s and %s",
                                types$typeString(varType),
                                types$typeString(valType)));
                }
            }
            Checker_declare(c, decl, NULL, c->pkg->scope, ast$ObjKind_VAL,
                    decl->value.name);
            break;
        }
    default:
        Checker_error(c, decl->pos, "unreachable");
    }
}

static void Checker_checkFile(Checker *c, ast$File *file) {
    for (int i = 0; file->imports[i] != NULL; i++) {
        types$Package *pkg = Checker_checkImport(c, file->imports[i]);
        utils$Slice_append(&c->pkg->imports, &pkg);
    }
    for (int i = 0; file->decls[i] != NULL; i++) {
        ast$ObjKind kind = ast$ObjKind_BAD;
        switch (file->decls[i]->kind) {
        case ast$DECL_FUNC:
            kind = ast$ObjKind_FUN;
            break;
        case ast$DECL_TYPEDEF:
            kind = ast$ObjKind_TYP;
            break;
        default:
            break;
        }
        if (kind) {
            Checker_declare(c, file->decls[i], NULL, c->pkg->scope, kind,
                    file->decls[i]->typedef_.name);
        }
    }
    for (int i = 0; file->decls[i] != NULL; i++) {
        Checker_checkDecl(c, file->decls[i]);
    }
}

extern types$Info *types$newInfo() {
    types$Info info = {
        .imports = utils$Map_make(sizeof(types$Package *)),
    };
    return esc(info);
}

extern types$Package *types$checkFile(types$Config *conf, const char *path,
        token$FileSet *fset, ast$File *file, types$Info *info) {
    if (info) {
        types$Package *pkg = NULL;
        utils$Map_get(&info->imports, path, &pkg);
        if (pkg) {
            return pkg;
        }
    } else {
        info = types$newInfo();
    }
    // sys$printf("checking %s\n", path);
    types$Package pkg = {
        .path = sys$strdup(path),
        .name = file->name ? sys$strdup(file->name->ident.name) : NULL,
        .scope = file->scope,
        .imports = {.size = sizeof(types$Package *)},
    };
    Checker c = {
        .info = info,
        .fset = fset,
        .conf = conf,
        .pkg = esc(pkg),
    };
    utils$Map_set(&info->imports, path, &c.pkg);
    Checker_checkFile(&c, file);
    return c.pkg;
}

extern types$Package *types$check(types$Config *conf, const char *path,
        token$FileSet *fset, ast$File **files, types$Info *info) {
    if (files == NULL) {
        files = parser$parseDir(fset, path, types$universe(), NULL);
    }
    assert(files[0] && !files[1]);
    return types$checkFile(conf, path, fset, files[0], info);
}
