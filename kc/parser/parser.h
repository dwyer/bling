#pragma once
#include "builtin/builtin.h"
#include "kc/ast/ast.h"
#include "kc/scanner/scanner.h"
#include "kc/token/token.h"

extern decl_t **parse_file(void);
extern scanner_t *scanner;
