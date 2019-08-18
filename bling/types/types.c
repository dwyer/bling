#include "bling/ast/ast.h"
#include "bling/emitter/emitter.h"
#include "bling/parser/parser.h"
#include "bling/types/types.h"
#include "paths/paths.h"
#include "sys/sys.h"

static char *constant_stringVal(ast$Expr *x) {
    // TODO move this to const pkg
    assert(x->type == ast$EXPR_BASIC_LIT);
    const char *lit = x->basic_lit.value;
    int n = strlen(lit) - 2;
    char *val = malloc(n + 1);
    for (int i = 0; i < n; i++) {
        val[i] = lit[i+1];
    }
    val[n] = '\0';
    return val;
}

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
    switch (expr->type) {
    case ast$EXPR_IDENT:
        if (expr->ident.obj == NULL) {
            panic("types$isType: unresolved identifier %s", expr->ident.name);
        }
        return expr->ident.obj->decl->type == ast$DECL_TYPEDEF;
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
        .type = ast$EXPR_IDENT,
        .ident = {
            .name = strdup(name),
        },
    };
    return esc(x);
}

static ast$Expr *types$makePtr(ast$Expr *type) {
    ast$Expr x = {
        .type = ast$EXPR_STAR,
        .pos = type->pos,
        .star = {
            .x = type,
        }
    };
    return esc(x);
}

static ast$Expr *getUnderlyingType(ast$Expr *ident) {
    ast$Decl *decl = ident->ident.obj->decl;
    if (decl->type != ast$DECL_TYPEDEF) {
        panic("not a type: %s", types$typeString(ident));
    }
    return decl->typedef_.type;
}

static ast$Expr *types$getBaseType(ast$Expr *type) {
    for (;;) {
        switch (type->type) {
        case ast$EXPR_IDENT:
            type = getUnderlyingType(type);
            break;
        case ast$EXPR_SELECTOR:
            type = type->selector.sel;
            break;
        case ast$TYPE_ARRAY:
        case ast$TYPE_ENUM:
        case ast$TYPE_NATIVE:
        case ast$TYPE_STRUCT:
            return type;
        default:
            panic("not a type: %s", types$typeString(type));
        }
    }
}

static bool types$isArithmetic(ast$Expr *type) {
    switch (type->type) {
    case ast$EXPR_IDENT:
        return types$isArithmetic(types$getBaseType(type));
    case ast$EXPR_STAR:
    case ast$TYPE_ENUM:
        return true;
    case ast$TYPE_NATIVE:
        return !streq(type->native.name, "void");
    default:
        return false;
    }
}

static bool types$isNative(ast$Expr *type, const char *name) {
    switch (type->type) {
    case ast$EXPR_IDENT:
        return streq(type->ident.name, name);
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
    assert(a);
    assert(b);
    if (a->type == ast$EXPR_SELECTOR) {
        a = a->selector.sel;
    }
    if (b->type == ast$EXPR_SELECTOR) {
        b = b->selector.sel;
    }
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
        panic("not implemented: %s == %s",
                types$typeString(a), types$typeString(b));
        return false;
    }
}

static bool types$isPointer(ast$Expr *t) {
    return t->type == ast$EXPR_STAR || t->type == ast$TYPE_ARRAY;
}

static ast$Expr *types$pointerBase(ast$Expr *t) {
    switch (t->type) {
    case ast$EXPR_STAR:
        return t->star.x;
    case ast$TYPE_ARRAY:
        return t->array.elt;
    default:
        panic("not a pointer: %s", types$typeString(t));
        return NULL;
    }
}

static bool types$areAssignable(ast$Expr *a, ast$Expr *b) {
    if (types$areIdentical(a, b)) {
        return true;
    }
    if (ast$isVoidPtr(a) || ast$isVoidPtr(b)) {
        return true;
    }
    if (types$isPointer(a) && types$isPointer(b)) {
        return types$areAssignable(types$pointerBase(a), types$pointerBase(b));
    }
    while (a->type == ast$EXPR_IDENT) {
        a = getUnderlyingType(a);
    }
    while (b->type == ast$EXPR_IDENT) {
        b = getUnderlyingType(b);
    }
    if (types$isNative(a, "bool") && types$isArithmetic(b)) {
        return true;
    }
    if (b->type == ast$TYPE_ENUM && types$isArithmetic(a)) {
        return true;
    }
    if (a->type == ast$TYPE_ENUM && types$isArithmetic(a)) {
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
    if (a->type == ast$EXPR_STAR && types$isNative(b, "int")) {
        return true;
    }
    return types$areIdentical(a, b);
}

static ast$ObjKind getDeclKind(ast$Decl *decl) {
    switch (decl->type) {
    case ast$DECL_FIELD:
    case ast$DECL_VALUE:
        return ast$ObjKind_VALUE;
    case ast$DECL_FUNC:
        return ast$ObjKind_FUNC;
    case ast$DECL_IMPORT:
        return ast$ObjKind_PKG;
    case ast$DECL_TYPEDEF:
        return ast$ObjKind_TYPE;
    default:
        return ast$ObjKind_BAD;
    }
}

static ast$Expr *getDeclName(ast$Decl *decl) {
    switch (decl->type) {
    case ast$DECL_FIELD:
        return decl->field.name;
    case ast$DECL_FUNC:
        return decl->func.name;
    case ast$DECL_IMPORT:
        return decl->imp.name;
    case ast$DECL_TYPEDEF:
        return decl->typedef_.name;
    case ast$DECL_VALUE:
        return decl->value.name;
    default:
        panic("bad decl: %s", types$declString(decl));
        return NULL;
    }
}

static void declareBuiltins(ast$Scope *s) {
    for (int i = 0; natives[i].name != NULL; i++) {
        ast$Expr _name = {
            .type = ast$EXPR_IDENT,
            .ident = {
                .name = strdup(natives[i].name),
            },
        };
        ast$Expr *name = esc(_name);
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
                .name = name,
                .type = esc(type),
            },
        };
        ast$Object *obj = ast$newObject(ast$ObjKind_TYPE, name->ident.name);
        obj->decl = esc(decl);
        name->ident.obj = obj;
        assert(ast$Scope_insert(s, obj) == NULL);
    }
}

ast$Scope *_universe = NULL;

extern ast$Scope *types$universe() {
    if (_universe == NULL) {
        _universe = ast$Scope_new(NULL);
        declareBuiltins(_universe);
        token$FileSet *fset = token$newFileSet();
        ast$File *file = parser$parseFile(fset, "builtin/builtin.bling");
        file->scope = _universe;
        types$Config conf = {.strict = true};
        types$checkFile(&conf, fset, file);
        free(file->decls);
        free(file);
    }
    return _universe;
}

static bool types$isLhs(ast$Expr *expr) {
    switch (expr->type) {
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
    switch (decl->type) {
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
        panic("unhandled decl: %s", types$declString(decl));
        return NULL;
    }
}

static ast$Decl *getStructFieldByName(ast$Expr *type, ast$Expr *name) {
    assert(type->type == ast$TYPE_STRUCT);
    assert(name->type == ast$EXPR_IDENT);
    for (int i = 0; type->struct_.fields && type->struct_.fields[i]; i++) {
        ast$Decl *field = type->struct_.fields[i];
        ast$Expr *fieldName = field->field.name;
        if (fieldName) {
            if (streq(name->ident.name, fieldName->ident.name)) {
                type = field->field.type;
                return field;
            }
        } else {
            return getStructFieldByName(field->field.type, name);
        }
    }
    return NULL;
}

static ast$Decl *getStructField(ast$Expr *type, int index) {
    assert(type->type == ast$TYPE_STRUCT);
    if (type->struct_.fields == NULL) {
        panic("incomplete field defn: %s", types$typeString(type));
    }
    for (int i = 0; type->struct_.fields[i]; i++) {
        if (i == index) {
            return type->struct_.fields[i];
        }
    }
    return NULL;
}

typedef struct {
    token$FileSet *fset;
    types$Config *conf;
    ast$Package pkg;
    ast$Expr *result;
    utils$Slice files;
    ast$Expr *typedefName;
    utils$Map scopes;
} Checker;

static void Checker_error(Checker *c, token$Pos pos, const char *msg) {
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
    assert(x->type == ast$EXPR_IDENT);
    assert(x->ident.obj == NULL);
    if (x->ident.pkg) {
        ast$Expr *pkg = x->ident.pkg;
        Checker_resolve(c, s, pkg);
        if (pkg->ident.obj->kind != ast$ObjKind_PKG) {
            Checker_error(c, pkg->pos,
                    sys$sprintf("not a pkg: %s", pkg->ident.name));
        }
        ast$Decl *decl = pkg->ident.obj->decl;
        assert(decl->type == ast$DECL_IMPORT);
        ast$Scope *t = decl->imp.scope;
        if (!t) {
            Checker_error(c, x->pos,
                    sys$sprintf("%s $ %s", pkg->ident.name, x->ident.name));
        }
        assert(t);
        s = t;
    }
    if (ast$resolve(s, x)) {
        return;
    }
    Checker_error(c, x->pos, sys$sprintf("unresolved: %s", x->ident.name));
}

static void Checker_declare(Checker *c, ast$Scope *s, ast$Decl *decl) {
    ast$Expr *ident = getDeclName(decl);
    if (ident->ident.obj != NULL) {
        Checker_error(c, decl->pos,
                sys$sprintf("already declared: %s", ident->ident.name));
    }
    if (ident->ident.pkg != NULL) {
        ast$Expr *pkg = ident->ident.pkg;
        Checker_resolve(c, s, pkg);
        ast$Object *pkgObj = pkg->ident.obj;
        ast$Decl *decl = pkgObj->decl;
        assert(decl->type == ast$DECL_IMPORT);
        s = decl->imp.scope;
        // panic("get pkg scope: %s $ %s", pkg->ident.name, ident->ident.name);
    }
    ast$ObjKind kind = getDeclKind(decl);
    ast$Object *obj = ast$newObject(kind, ident->ident.name);
    obj->decl = decl;
    obj->scope = s;
    obj->pkg = s->pkg;
    ident->ident.obj = obj;
    ast$Object *alt = ast$Scope_insert(s, obj);
    if (alt != NULL) {
        if (alt->kind != kind) {
            Checker_error(c, decl->pos,
                    sys$sprintf("incompatible redefinition of `%s`: %s",
                        types$declString(obj->decl),
                        types$declString(decl)));
        }
        bool redecl = false;
        switch (kind) {
        case ast$ObjKind_BAD:
            panic("unreachable");
            break;
        case ast$ObjKind_FUNC:
            redecl =
                (alt->decl->func.body == NULL || decl->func.body == NULL) &&
                types$areIdentical(alt->decl->func.type, decl->func.type);
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
                !types$areIdentical(alt->decl->value.type, decl->value.value);
            break;
        }
        if (!redecl) {
            Checker_error(c, decl->pos,
                    sys$sprintf("already declared: %s", ident->ident.name));
        }
        alt->decl = decl;
    }
}

static void Checker_openScope(Checker *c) {
    c->pkg.scope = ast$Scope_new(c->pkg.scope);
}

static void Checker_closeScope(Checker *c) {
    // ast$Scope *inner = c->pkg.scope;
    c->pkg.scope = c->pkg.scope->outer;
    // ast$Scope_free(inner);
}

static ast$Expr *Checker_checkExpr(Checker *c, ast$Expr *expr);

static void Checker_checkType(Checker *c, ast$Expr *t) {
    assert(t);
    switch (t->type) {

    case ast$EXPR_IDENT:
        Checker_resolve(c, c->pkg.scope, t);
        break;

    case ast$EXPR_SELECTOR:
        {
            ast$Expr *type = Checker_checkExpr(c, t->selector.x);
            assert(type == NULL);
            assert(ast$isIdent(t->selector.x));
            assert(t->selector.x->ident.obj->kind == ast$ObjKind_PKG);
            ast$Scope *oldScope = c->pkg.scope;
            c->pkg.scope = t->selector.x->ident.obj->decl->imp.scope;
            Checker_checkType(c, t->selector.sel);
            c->pkg.scope = oldScope;
        }
        break;

    case ast$TYPE_ARRAY:
        Checker_checkType(c, t->array.elt);
        if (t->array.len) {
            ast$Expr *len = Checker_checkExpr(c, t->array.len);
            (void)len; // TODO assert that len resolves to int
        }
        break;

    case ast$TYPE_ENUM:
        for (int i = 0; t->enum_.enums[i]; i++) {
            ast$Decl *decl = t->enum_.enums[i];
            if (c->typedefName) {
                decl->value.type = c->typedefName;
            }
            Checker_declare(c, c->pkg.scope, decl);
        }
        break;

    case ast$TYPE_FUNC:
        for (int i = 0; t->func.params && t->func.params[i]; i++) {
            ast$Decl *param = t->func.params[i];
            if (param->type == ast$DECL_ELLIPSIS) {
                continue;
            }
            assert(param->type == ast$DECL_FIELD);
            Checker_checkType(c, param->field.type);
        }
        if (t->func.result) {
            Checker_checkType(c, t->func.result);
        }
        break;

    case ast$EXPR_STAR:
        Checker_checkType(c, t->star.x);
        break;

    case ast$TYPE_STRUCT:
        if (t->struct_.fields) {
            Checker_openScope(c);
            for (int i = 0; t->struct_.fields[i]; i++) {
                ast$Decl *field = t->struct_.fields[i];
                Checker_checkType(c, field->field.type);
                if (field->field.name) {
                    Checker_declare(c, c->pkg.scope, field);
                }
            }
            Checker_closeScope(c);
        }
        break;

    default:
        Checker_error(c, t->pos, "unknown type");
        break;
    }
}

static ast$Expr *Checker_checkIdent(Checker *c, ast$Expr *expr) {
    assert(expr->type == ast$EXPR_IDENT);
    if (expr->ident.obj == NULL) {
        Checker_error(c, expr->pos, "unresolved identifier");
    }
    return getDeclType(expr->ident.obj->decl);
}

static bool types$isInteger(ast$Expr *x) {
    switch (x->type) {
    case ast$EXPR_IDENT:
        return types$isInteger(x->ident.obj->decl->typedef_.type);
    case ast$TYPE_ENUM:
    case ast$TYPE_NATIVE:
        return true;
    default:
        return false;
    }
}

static void Checker_checkArrayLit(Checker *c, ast$Expr *x) {
    ast$Expr *baseT = types$getBaseType(x->compound.type);
    for (int i = 0; x->compound.list[i]; i++) {
        ast$Expr *elt = x->compound.list[i];
        if (elt->type == ast$EXPR_KEY_VALUE) {
            elt->key_value.isArray = true;
            ast$Expr *indexT = Checker_checkExpr(c, elt->key_value.key);
            if (!types$isInteger(indexT)) {
                Checker_error(c, elt->pos,
                        sys$sprintf("not a valid index: %s",
                            types$exprString(elt->key_value.key)));
            }
            elt = elt->key_value.value;
        }
        if (elt->type == ast$EXPR_COMPOUND) {
            if (elt->compound.type == NULL) {
                elt->compound.type = baseT->array.elt;
            }
        }
        ast$Expr *eltT = Checker_checkExpr(c, elt);
        (void)eltT;
    }
}

static void Checker_checkStructLit(Checker *c, ast$Expr *x) {
    assert(x->compound.type);
    ast$Expr *baseT = types$getBaseType(x->compound.type);
    bool expectKV = false;
    for (int i = 0; x->compound.list[i]; i++) {
        ast$Expr *elt = x->compound.list[i];
        ast$Expr *fieldT = NULL;
        if (elt->type == ast$EXPR_KEY_VALUE) {
            elt->key_value.isArray = false;
            expectKV = true;
            ast$Expr *key = elt->key_value.key;
            if (!ast$isIdent(key)) {
                Checker_error(c, x->pos, "key must be an identifier");
            }
            ast$Decl *field = getStructFieldByName(baseT, key);
            if (field == NULL) {
                Checker_error(c, key->pos,
                        sys$sprintf("struct `%s` has no field `%s`",
                            types$typeString(x->compound.type),
                            types$exprString(key)));
            }
            key->ident.obj = field->field.name->ident.obj;
            fieldT = field->field.type;
            elt = elt->key_value.value;
        } else {
            if (expectKV) {
                Checker_error(c, x->pos, "expected a key/value expr");
            }
            ast$Decl *field = getStructField(baseT, i);
            fieldT = field->field.type;
        }
        if (elt->type == ast$EXPR_COMPOUND) {
            if (elt->compound.type == NULL) {
                elt->compound.type = fieldT;
            }
        }
        ast$Expr *eltT = Checker_checkExpr(c, elt);
        if (!types$areAssignable(fieldT, eltT)) {
            Checker_error(c, elt->pos, sys$sprintf(
                        "cannot init field of type `%s` with value of type `%s`",
                        types$typeString(fieldT),
                        types$typeString(eltT)));
        }
    }
}

static void Checker_checkCompositeLit(Checker *c, ast$Expr *x) {
    ast$Expr *t = x->compound.type;
    assert(t);
    ast$Expr *baseT = types$getBaseType(t);
    switch (baseT->type) {
    case ast$TYPE_ARRAY:
        Checker_checkArrayLit(c, x);
        break;
    case ast$TYPE_STRUCT:
        Checker_checkStructLit(c, x);
        break;
    default:
        Checker_error(c, x->pos, "composite type must be an array or a struct");
        break;
    }
}

static ast$Expr *Checker_checkExpr(Checker *c, ast$Expr *expr) {
    assert(expr);
    switch (expr->type) {

    case ast$EXPR_BINARY:
        {
            ast$Expr *typ1 = Checker_checkExpr(c, expr->binary.x);
            ast$Expr *typ2 = Checker_checkExpr(c, expr->binary.y);
            if (!types$areComparable(typ1, typ2)) {
                Checker_error(c, expr->pos,
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
                typ1 = types$makeIdent("bool");
                Checker_checkType(c, typ1);
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
                type = types$makeIdent("char");
                break;
            case token$FLOAT:
                type = types$makeIdent("float");
                break;
            case token$INT:
                type = types$makeIdent("int");
                break;
            case token$STRING:
                type = types$makePtr(types$makeIdent("char"));
                break;
            default:
                Checker_error(c, expr->pos, "unreachable");
                break;
            }
            Checker_checkType(c, type);
            return type;
        }

    case ast$EXPR_CALL:
        {
            ast$Expr *func = expr->call.func;
            ast$Expr *type = Checker_checkExpr(c, func);
            if (func->type == ast$EXPR_IDENT) {
                if (streq(func->ident.name, "esc")) {
                    assert(expr->call.args[0] && !expr->call.args[1]);
                    ast$Expr *type = Checker_checkExpr(c, expr->call.args[0]);
                    return types$makePtr(type);
                }
            }
            if (type->type == ast$EXPR_STAR) {
                type = type->star.x;
            }
            if (type->type != ast$TYPE_FUNC) {
                Checker_error(c, expr->pos, sys$sprintf("`%s` is not a func",
                            types$exprString(expr->call.func)));
            }
            int j = 0;
            for (int i = 0; expr->call.args[i]; i++) {
                ast$Decl *param = type->func.params[j];
                ast$Expr *type = Checker_checkExpr(c, expr->call.args[i]);
                if (param->type == ast$DECL_FIELD) {
                    if (!types$areAssignable(param->field.type, type)) {
                        Checker_error(c, expr->pos, sys$sprintf(
                                    "not assignable: %s and %s",
                                    types$typeString(param->field.type),
                                    types$typeString(type)));
                    }
                    j++;
                } else {
                    assert(param->type == ast$DECL_ELLIPSIS);
                }
            }
            return type->func.result;

        }

    case ast$EXPR_COMPOUND:
        if (expr->compound.type == NULL) {
            Checker_error(c, expr->pos, "untyped compound expr");
        }
        Checker_checkCompositeLit(c, expr);
        return expr->compound.type;

    case ast$EXPR_CAST:
        Checker_checkType(c, expr->cast.type);
        if (expr->cast.expr->type == ast$EXPR_COMPOUND) {
            expr->cast.expr->compound.type = expr->cast.type;
        }
        Checker_checkExpr(c, expr->cast.expr);
        return expr->cast.type;

    case ast$EXPR_IDENT:
        Checker_resolve(c, c->pkg.scope, expr);
        return Checker_checkIdent(c, expr);

    case ast$EXPR_INDEX:
        {
            ast$Expr *type = Checker_checkExpr(c, expr->index.x);
            switch (type->type) {
            case ast$TYPE_ARRAY:
                type = type->array.elt;
                break;
            case ast$EXPR_STAR:
                type = type->star.x;
                break;
            default:
                Checker_error(c, expr->pos,
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
                assert(x->type == ast$EXPR_IDENT);
                ast$Decl *decl = x->ident.obj->decl;
                assert(decl->type == ast$DECL_IMPORT);
                ast$Scope *oldScope = c->pkg.scope;
                c->pkg.scope = decl->imp.scope;
                type = Checker_checkExpr(c, expr->selector.sel);
                c->pkg.scope = oldScope;
                expr->selector.tok = token$DOLLAR;
                return type;
            }
            if (type->type == ast$EXPR_STAR) {
                expr->selector.tok = token$ARROW;
                type = type->star.x;
            } else {
                assert(expr->selector.tok != token$ARROW);
            }
            type = types$getBaseType(type);
            ast$Decl *field = getStructFieldByName(type, expr->selector.sel);
            if (field == NULL) {
                Checker_error(c, expr->pos,
                        sys$sprintf("struct `%s` (`%s`) has no field `%s`",
                            types$exprString(expr->selector.x),
                            types$typeString(type),
                            expr->selector.sel->ident.name));
            }
            expr->selector.sel->ident.obj = field->field.name->ident.obj;
            return field->field.type;
        }

    case ast$EXPR_SIZEOF:
        {
            Checker_checkType(c, expr->sizeof_.x);
            ast$Expr *ident = types$makeIdent("size_t");
            Checker_resolve(c, c->pkg.scope, ident);
            return ident;
        }

    case ast$EXPR_STAR:
        {
            ast$Expr *type = Checker_checkExpr(c, expr->star.x);
            switch (type->type) {
            case ast$EXPR_STAR:
                return type->star.x;
            case ast$TYPE_ARRAY:
                return type->array.elt;
            default:
                Checker_error(c, expr->pos,
                        sys$sprintf("derefencing a non-pointer `%s`",
                            types$typeString(type)));
                return NULL;
            }
        }

    case ast$EXPR_UNARY:
        {
            ast$Expr *type = Checker_checkExpr(c, expr->unary.x);
            if (expr->unary.op == token$AND) {
                if (!types$isLhs(expr->unary.x)) {
                    Checker_error(c, expr->pos,
                            sys$sprintf("invalid lvalue `%s`",
                                types$exprString(expr->unary.x)));
                }
                return types$makePtr(type);
            }
            return type;
        }

    default:
        Checker_error(c, expr->pos, "unknown expr");
        return NULL;
    }
}

static void Checker_checkDecl(Checker *c, ast$Decl *decl);

static void Checker_checkStmt(Checker *c, ast$Stmt *stmt) {
    switch (stmt->type) {
    case ast$STMT_ASSIGN:
        {
            if (!types$isLhs(stmt->assign.x)) {
                Checker_error(c, stmt->pos,
                        sys$sprintf("invalid lvalue `%s`",
                            types$exprString(stmt->assign.x)));
            }
            ast$Expr *a = Checker_checkExpr(c, stmt->assign.x);
            if (a->is_const) {
                Checker_error(c, stmt->pos, "cannot assign to const var");
            }
            ast$Expr *b = Checker_checkExpr(c, stmt->assign.y);
            if (!types$areAssignable(a, b)) {
                Checker_error(c, stmt->pos,
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
                Checker_error(c, stmt->pos, "returning value in void function");
            }
            if (!types$areAssignable(a, b)) {
                Checker_error(c, stmt->pos,
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
                assert(clause->type == ast$STMT_CASE);
                for (int j = 0; clause->case_.exprs && clause->case_.exprs[j]; j++) {
                    ast$Expr *type2 = Checker_checkExpr(c, clause->case_.exprs[j]);
                    if (!types$areComparable(type1, type2)) {
                        Checker_error(c, stmt->pos,
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
        Checker_error(c, stmt->pos, "unknown stmt");
    }
}

static void Checker_checkFile(Checker *c, ast$File *file);

static void Checker_checkImport(Checker *c, ast$Decl *imp) {
    char *path = constant_stringVal(imp->imp.path);
    if (imp->imp.name == NULL) {
        char *base = paths$base(path);
        imp->imp.name = types$makeIdent(base);
    }

    ast$Scope *oldScope = NULL;
    utils$Map_get(&c->scopes, path, &oldScope);
    if (oldScope) {
        imp->imp.scope = oldScope;
        Checker_declare(c, c->pkg.scope, imp);
        free(path);
        return;
    }

    imp->imp.scope = ast$Scope_new(types$universe());
    utils$Map_set(&c->scopes, path, &imp->imp.scope);

    Checker_declare(c, c->pkg.scope, imp);

    oldScope = c->pkg.scope;
    c->pkg.scope = imp->imp.scope;

    utils$Error *err = NULL;
    token$FileSet *fset = token$newFileSet();
    ast$File **files = parser$parseDir(fset, path, &err);
    if (err) {
        Checker_error(c, imp->pos, sys$sprintf("%s: %s", path, err->error));
    }
    for (int i = 0; files[i]; i++) {
        Checker_checkFile(c, files[i]);
    }

    c->pkg.scope = oldScope;
}

static void Checker_checkDecl(Checker *c, ast$Decl *decl) {
    switch (decl->type) {
    case ast$DECL_FUNC:
        Checker_checkType(c, decl->func.type);
        Checker_declare(c, c->pkg.scope, decl);
        break;
    case ast$DECL_PRAGMA:
        break;
    case ast$DECL_TYPEDEF:
        c->typedefName = decl->typedef_.name;
        Checker_checkType(c, decl->typedef_.type);
        c->typedefName = NULL;
        Checker_declare(c, c->pkg.scope, decl);
        break;
    case ast$DECL_VALUE:
        {
            ast$Expr *valType = NULL;
            if (decl->value.type != NULL) {
                Checker_checkType(c, decl->value.type);
            }
            if (decl->value.value != NULL) {
                if (decl->value.value->type == ast$EXPR_COMPOUND) {
                    if (decl->value.type == NULL) {
                        // TODO resolve this restriction by enforcing T{}.
                        Checker_error(c, decl->pos, "cannot assign short var decls with composite type");
                    }
                    decl->value.value->compound.type = decl->value.type;
                }
                valType = Checker_checkExpr(c, decl->value.value);
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
            Checker_declare(c, c->pkg.scope, decl);
            break;
        }
    default:
        Checker_error(c, decl->pos, "unreachable");
    }
}

static void Checker_checkFunc(Checker *c, ast$Decl *decl) {
    if (decl->type == ast$DECL_FUNC && decl->func.body) {
        Checker_openScope(c);
        ast$Expr *type = decl->func.type;
        for (int i = 0; type->func.params && type->func.params[i]; i++) {
            ast$Decl *param = type->func.params[i];
            assert(param->type == ast$DECL_FIELD);
            if (param->field.name) {
                Checker_declare(c, c->pkg.scope, param);
            }
        }
        c->result = decl->func.type->func.result;
        // walk the block manually to avoid opening a new scope
        for (int i = 0; decl->func.body->block.stmts[i]; i++) {
            Checker_checkStmt(c, decl->func.body->block.stmts[i]);
        }
        c->result = NULL;
        Checker_closeScope(c);
    }
}

static void Checker_checkFile(Checker *c, ast$File *file) {
    if (file->name) {
        c->pkg.scope->pkg = file->name->ident.name;
    }
    for (int i = 0; file->imports[i] != NULL; i++) {
        Checker_checkImport(c, file->imports[i]);
    }
    utils$Slice_append(&c->files, &file);
    if (c->conf->strict) {
        for (int i = 0; file->decls[i] != NULL; i++) {
            Checker_checkDecl(c, file->decls[i]);
        }
        for (int i = 0; file->decls[i] != NULL; i++) {
            Checker_checkFunc(c, file->decls[i]);
        }
    }
}

extern ast$Package types$checkFile(types$Config *conf, token$FileSet *fset,
        ast$File *file) {
    if (file->scope == NULL) {
        file->scope = ast$Scope_new(types$universe());
    }
    Checker c = {
        .fset = fset,
        .conf = conf,
        .pkg = {
            .scope = file->scope,
        },
        .files = utils$Slice_init(sizeof(ast$File *)),
        .scopes = utils$Map_init(sizeof(ast$Scope *)),
    };
    Checker_checkFile(&c, file);
    c.pkg.files = utils$Slice_to_nil_array(c.files);
    return c.pkg;
}
