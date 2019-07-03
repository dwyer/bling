#pragma once
#include "builtin/builtin.h"
#include "subc/ast/ast.h"

typedef struct {
    FILE *fp;
    int indent;
} emitter_t;

extern void emitter_emit_file(emitter_t *e, file_t *file);
