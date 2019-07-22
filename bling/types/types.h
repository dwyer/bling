#pragma once
#include "bling/ast/ast.h"

$import("bling/ast");
$import("bling/emitter");

extern void declare_builtins(scope_t *s);

extern bool types_isIdent(expr_t *expr);
extern bool types_isVoid(expr_t *type);
extern bool types_isVoidPtr(expr_t *type);

extern char *types_exprString(expr_t *expr);
extern char *types_stmtString(stmt_t *stmt);
extern char *types_typeString(expr_t *expr);

typedef struct {
    bool strict;
} config_t;

extern package_t types_checkFile(config_t *conf, file_t *file);
