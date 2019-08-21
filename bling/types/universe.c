#include "bling/types/types.h"

#include "bling/parser/parser.h"

typedef enum {
    _assert,
    _esc,
    _free,
    _malloc,
    _panic,
    _print,
    _strdup,
    _streq,
    _strlen,
    numBuiltinIds,
} builtinId;

typedef enum {
    expression,
    statement,
} exprKind;

struct {
    char *name;
    int nargs;
    bool variadic;
    exprKind kind;
} predeclareFuncs[] = {
    [_assert]   = {"assert", 1, false, statement},
    [_esc]      = {"esc", 1, false, expression},
    [_free]     = {"free", 1, false, statement},
    [_malloc]   = {"malloc", 1, false, expression},
    [_panic]    = {"panic", 1, true, statement},
    [_print]    = {"print", 1, false, statement},
    [_strdup]   = {"strdup", 1, false, expression},
    [_streq]    = {"strdup", 2, false, expression},
    [_strlen]   = {"strdup", 1, false, expression},
};

void defPredeclareFuncs() {
    for (int i = 0; i < numBuiltinIds; i++) {
    }
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

static void declareBuiltins(ast$Scope *s) {
    for (int i = 0; natives[i].name != NULL; i++) {
        ast$Expr _name = {
            .kind = ast$EXPR_IDENT,
            .ident = {
                .name = strdup(natives[i].name),
            },
        };
        ast$Expr *name = esc(_name);
        ast$Expr type = {
            .kind = ast$TYPE_NATIVE,
            .native = {
                .name = strdup(natives[i].name),
                .size = natives[i].size,
            },
        };
        ast$Decl decl = {
            .kind = ast$DECL_TYPEDEF,
            .typedef_ = {
                .name = name,
                .type = esc(type),
            },
        };
        ast$Object *obj = ast$newObject(ast$ObjKind_TYP, name->ident.name);
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
        ast$File *file = parser$parseFile(fset, "builtin/builtin.bling", _universe);
        _universe = file->scope;
        types$Config conf = {.strict = true};
        types$checkFile(&conf, "builtin", fset, file, NULL);
        free(file->decls);
        free(file);
    }
    return _universe;
}
