#pragma once
#include "bling/ast/ast.h"
#include "bytes/bytes.h"

import("bling/ast");
import("bytes");

typedef struct {
    bytes$Buffer buf;
    int indent;
    bool skipSemi;
} emitter_t;

extern char *emitter_string(emitter_t *e);

extern void emit_string(emitter_t *p, const char *s);
extern void emit_space(emitter_t *e);
extern void emit_newline(emitter_t *e);
extern void emit_tabs(emitter_t *e);
extern void emit_token(emitter_t *e, token$Token tok);

extern void print_decl(emitter_t *p, ast$Decl *decl);
extern void print_expr(emitter_t *p, ast$Expr *expr);
extern void print_stmt(emitter_t *p, ast$Stmt *stmt);
extern void print_type(emitter_t *p, ast$Expr *type);
extern void printer_print_file(emitter_t *e, ast$File *file);
