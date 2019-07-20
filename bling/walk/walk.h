#pragma once
#include "bling/ast/ast.h"

$import("bling/ast");
$import("bling/emitter");

extern void declare_builtins(scope_t *s);
extern void walk_file(file_t *file);
