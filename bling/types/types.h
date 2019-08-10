#pragma once
#include "bling/ast/ast.h"

package(types);

import("bling/ast");
import("bling/emitter");
import("bling/parser");
import("bling/token");
import("paths");
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
} types$Config;

extern ast$Package types$checkFile(types$Config *conf, ast$File *file);
