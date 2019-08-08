#pragma once
#include "bling/ast/ast.h"

import("bling/ast");
import("bling/emitter");
import("bling/parser");

extern ast$Scope *types_universe();

extern bool types_isIdent(ast$Expr *expr);
extern bool types_isVoid(ast$Expr *type);
extern bool types_isVoidPtr(ast$Expr *type);

extern char *types_exprString(ast$Expr *expr);
extern char *types_stmtString(ast$Stmt *stmt);
extern char *types_typeString(ast$Expr *expr);

typedef struct {
    bool strict;
} config_t;

extern ast$Package types_checkFile(config_t *conf, ast$File *file);
