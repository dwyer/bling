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

extern bool types$isIdent(ast$Expr *expr);
extern bool types$isVoid(ast$Expr *type);
extern bool types$isVoidPtr(ast$Expr *type);

extern char *types$exprString(ast$Expr *expr);
extern char *types$stmtString(ast$Stmt *stmt);
extern char *types$typeString(ast$Expr *expr);

typedef struct {
    bool strict;
    bool cMode;
    bool ignoreFuncBodies;
} types$Config;

typedef struct {
    utils$Map scopes;
} types$Info;

extern types$Info *types$newInfo();

typedef struct {
    char *path;
    char *name;
    ast$Scope *scope;
    utils$Slice imports;
} types$Package;

extern types$Package *types$checkFile(types$Config *conf, const char *path,
        token$FileSet *fset, ast$File *file, types$Info *info);
extern types$Package *types$check(types$Config *conf, const char *path,
        token$FileSet *fset, ast$File **files, types$Info *info);

extern char *types$constant_stringVal(ast$Expr *x); // TODO move this to const pkg
