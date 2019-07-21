#pragma once
#include "bling/ast/ast.h"
#include "strings/strings.h"

$import("bling/ast");
$import("strings");

typedef struct {
    strings_Builder builder;
    int indent;
    bool skipSemi;
} emitter_t;

extern void emit_string(emitter_t *p, const char *s);
extern void emit_space(emitter_t *e);
extern void emit_newline(emitter_t *e);
extern void emit_tabs(emitter_t *e);
extern void emit_token(emitter_t *e, token_t tok);

extern void print_decl(emitter_t *p, decl_t *decl);
extern void print_expr(emitter_t *p, expr_t *expr);
extern void print_stmt(emitter_t *p, stmt_t *stmt);
extern void print_type(emitter_t *p, expr_t *type);
extern void printer_print_file(emitter_t *e, file_t *file);
