#include "bling/types/types.h"

#include "bling/parser/parser.h"
#include "sys/sys.h"

static ast$Scope *_universe = NULL;

static const types$Basic predeclaredTypes[] = {
    [types$INVALID] = {types$INVALID, 0, "invalid type"},
    [types$VOID] = {types$VOID, types$IS_UNTYPED, "void"},
    [types$BOOL] = {types$BOOL, types$IS_BOOLEAN, "bool"},
    [types$CHAR] = {types$CHAR, types$IS_INTEGER, "char"},
    [types$INT] = {types$INT, types$IS_INTEGER, "int"},
    [types$INT8] = {types$INT8, types$IS_INTEGER, "i8"},
    [types$INT16] = {types$INT16, types$IS_INTEGER, "i16"},
    [types$INT32] = {types$INT32, types$IS_INTEGER, "i32"},
    [types$INT64] = {types$INT64, types$IS_INTEGER, "i64"},
    [types$UINT] = {types$UINT, types$IS_INTEGER | types$IS_UNSIGNED, "uint"},
    [types$UINT8] = {types$UINT8, types$IS_INTEGER | types$IS_UNSIGNED, "u8"},
    [types$UINT16] = {types$UINT16, types$IS_INTEGER | types$IS_UNSIGNED, "u16"},
    [types$UINT32] = {types$UINT32, types$IS_INTEGER | types$IS_UNSIGNED, "u32"},
    [types$UINT64] = {types$UINT64, types$IS_INTEGER | types$IS_UNSIGNED, "u64"},
    [types$UINTPTR] = {types$UINTPTR, types$IS_INTEGER | types$IS_UNSIGNED, "uintptr"},
    [types$FLOAT32] = {types$FLOAT32, types$IS_FLOAT, "float"},
    [types$FLOAT64] = {types$FLOAT64, types$IS_FLOAT, "double"},
    // [types$STRING] = {types$STRING, types$IS_STRING, "string"},
    [types$VOID_POINTER] = {types$VOID_POINTER, 0, "voidptr"},
};

static void defPredeclaredTypes() {
    for (int i = 0; i < types$UNSAFE_POINTER; i++) {
        if (!predeclaredTypes[i].name) {
            continue;
        }
        ast$Object *obj = ast$newObject(ast$ObjKind_TYP,
                predeclaredTypes[i].name);
        ast$Expr name = {
            .kind = ast$EXPR_IDENT,
            .ident = {
                .name = sys$strdup(predeclaredTypes[i].name),
                .obj = obj,
            },
        };
        ast$Expr type = {
            .kind = ast$TYPE_NATIVE,
            .native = {
                .kind = predeclaredTypes[i].kind,
                .info = predeclaredTypes[i].info,
                .name = sys$strdup(predeclaredTypes[i].name),
            },
        };
        ast$Decl decl = {
            .kind = ast$DECL_TYPEDEF,
            .typedef_ = {
                .name = esc(name),
                .type = esc(type),
            },
        };
        obj->decl = esc(decl);
        assert(ast$Scope_insert(_universe, obj) == NULL);
    }
}

typedef enum {
    _assert,
    _mapmake,
    _panic,
    _print,
    numBuiltinIds,
} builtinId;

typedef enum {
    expression,
    statement,
} exprKind;

static struct {
    char *name;
    int nargs;
    bool variadic;
    exprKind kind;
} predeclareFuncs[] = {
    [_assert]   = {"assert", 1, false, statement},
    [_mapmake]  = {"mapmake", 1, false, expression},
    [_panic]    = {"panic", 1, false, statement},
    [_print]    = {"print", 1, false, statement},
};

static void defPredeclaredFuncs() {
    for (int i = 0; i < numBuiltinIds; i++) {
        ast$Expr name = {
            .kind = ast$EXPR_IDENT,
            .ident = {
                .name = sys$strdup(predeclareFuncs[i].name),
            },
        };
        ast$Expr type = {
            .kind = ast$TYPE_BUILTIN,
            .builtin = {
                .name = sys$strdup(predeclareFuncs[i].name),
                .nargs = predeclareFuncs[i].nargs,
                .variadic = predeclareFuncs[i].variadic,
                .isExpr = predeclareFuncs[i].kind == expression,
            },
        };
        ast$Decl decl = {
            .kind = ast$DECL_FUNC,
            .func = {
                .name = esc(name),
                .type = esc(type),
            },
        };
        ast$Object *obj = ast$newObject(ast$ObjKind_FUN, name.ident.name);
        obj->decl = esc(decl);
        obj->decl->func.name->ident.obj = obj;
        assert(ast$Scope_insert(_universe, obj) == NULL);
    }
}

static struct {
    char *name;
    char *type;
    int value;
} predeclareConsts[] = {
    {"NULL", "voidptr", 0},
    {"false", "bool", 0},
    {"true", "bool", 1},
    {NULL},
};

static void defPredeclaredConsts() {
    for (int i = 0; predeclareConsts[i].name; i++) {
        ast$Object *obj = ast$Scope_lookup(_universe,
                predeclareConsts[i].type);
        ast$Expr name = {
            .kind = ast$EXPR_IDENT,
            .ident = {
                .name = sys$strdup(predeclareConsts[i].name),
            },
        };
        ast$Expr expr = {
            .kind = ast$EXPR_CONST,
            .const_ = {
                .name = sys$strdup(predeclareConsts[i].name),
                .type = obj->decl->typedef_.name,
                .value = predeclareConsts[i].value,
            },
        };
        ast$Decl decl = {
            .kind = ast$DECL_VALUE,
            .value = {
                .name = esc(name),
                .type = obj->decl->typedef_.name,
                .value = esc(expr),
            },
        };
        obj = ast$newObject(ast$ObjKind_CON, name.ident.name);
        obj->decl = esc(decl);
        obj->decl->value.name->ident.obj = obj;
        assert(ast$Scope_insert(_universe, obj) == NULL);
    }
}

extern ast$Scope *types$universe() {
    if (_universe == NULL) {
        _universe = ast$Scope_new(NULL);
        defPredeclaredTypes();
        defPredeclaredConsts();
        defPredeclaredFuncs();
    }
    return _universe;
}
