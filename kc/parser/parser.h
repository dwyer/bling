#pragma once
#include "builtin/builtin.h"
#include "io/ioutil/ioutil.h"
#include "kc/ast/ast.h"
#include "kc/scanner/scanner.h"
#include "kc/token/token.h"

extern file_t *parser_parse_file(char *filename, scope_t *pkg_scope);
