#pragma once
#include "kc.h"
#include "ast.h"

void emit_decl(decl_t *decl);
void emit_expr(expr_t *expr);
void emit_type(expr_t *type, expr_t *name);

extern FILE *emit_fp;
