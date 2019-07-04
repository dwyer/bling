#pragma once
#include "builtin/builtin.h"
#include "bling/ast/ast.h"

typedef struct {
    FILE *fp;
    int indent;
} printer_t;

extern void printer_print_file(printer_t *e, file_t *file);
