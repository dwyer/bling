#pragma once
#include "bling/ast/ast.h"

package(types);

import("bling/ast");
import("bling/emitter");
import("bling/parser");
import("bling/token");
import("sys");
import("utils");

extern ast$Scope *types$universe();

extern ast$Expr *types$getType(ast$Expr *x);
extern ast$Expr *types$getBaseType(ast$Expr *type);

extern bool types$isType(ast$Expr *expr);
extern bool types$isIdent(ast$Expr *expr);
extern bool types$isVoid(ast$Expr *type);
extern bool types$isVoidPtr(ast$Expr *type);
extern bool types$isDynamicArray(ast$Expr *t);

extern char *types$exprString(ast$Expr *expr);
extern char *types$stmtString(ast$Stmt *stmt);
extern char *types$typeString(ast$Expr *expr);

typedef struct {
    bool strict;
    bool cMode;
    bool ignoreFuncBodies;
} types$Config;

typedef struct types$Package types$Package;

typedef struct types$Package {
    char *path;
    char *name;
    ast$Scope *scope;
    array(types$Package *) imports;
} types$Package;

typedef struct {
    map(types$Package *) imports; // map of types$Package
} types$Info;

extern types$Info *types$newInfo();

extern types$Package *types$checkFile(types$Config *conf, const char *path,
        token$FileSet *fset, ast$File *file, types$Info *info);
extern types$Package *types$check(types$Config *conf, const char *path,
        token$FileSet *fset, ast$File **files, types$Info *info);

extern char *types$constant_stringVal(ast$Expr *x); // TODO move this to const pkg

typedef enum {
    types$INVALID,

    types$VOID,
    types$BOOL,
    types$CHAR,
    types$INT,
    types$INT8,
    types$INT16,
    types$INT32,
    types$INT64,
    types$UINT,
    types$UINT8,
    types$UINT16,
    types$UINT32,
    types$UINT64,
    types$UINTPTR,
    types$FLOAT32,
    types$FLOAT64,
    // types$COMPLEX64,
    // types$COMPLEX128,
    // types$STRING,
    types$VOID_POINTER,
    types$UNSAFE_POINTER,

} types$BasicKind;

typedef enum {

    types$IS_BOOLEAN    = 1 << 0,
    types$IS_INTEGER    = 1 << 1,
    types$IS_UNSIGNED   = 1 << 2,
    types$IS_FLOAT      = 1 << 3,
    types$IS_COMPLEX    = 1 << 4,
    types$IS_STRING     = 1 << 5,
    types$IS_UNTYPED    = 1 << 6,

    types$IS_ORDERED    = types$IS_INTEGER | types$IS_FLOAT | types$IS_STRING,
    types$IS_NUMERIC    = types$IS_INTEGER | types$IS_FLOAT | types$IS_COMPLEX,
    types$IS_CONST_EXPR = types$IS_BOOLEAN | types$IS_NUMERIC | types$IS_STRING,

} types$BasicInfo;

typedef struct {
    types$BasicKind kind;
    types$BasicInfo info;
    char *name;
} types$Basic;

typedef enum {
    types$APPEND,
    types$ASSERT,
    types$LEN,
    types$MAKEARRAY,
    types$MAKEMAP,
    types$PANIC,
    types$PRINT,
    types$numBuiltinIds,
} types$builtinId;

typedef enum {
    expression,
    statement,
} types$exprKind;
