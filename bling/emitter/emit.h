#pragma once
#include "bling/ast/ast.h"
#include "os/os.h"

$import("bling/ast");
$import("os");

typedef struct {
    os_File *file;
    int indent;
} emitter_t;

extern void emit_string(emitter_t *p, const char *s);
extern void emit_space(emitter_t *e);
extern void emit_newline(emitter_t *e);
extern void emit_tabs(emitter_t *e);
extern void emit_token(emitter_t *e, token_t tok);

extern void printer_print_file(emitter_t *e, file_t *file);
