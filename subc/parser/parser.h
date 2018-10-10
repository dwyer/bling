#pragma once
#include "builtin/builtin.h"
#include "io/ioutil/ioutil.h"
#include "subc/ast/ast.h"
#include "subc/scanner/scanner.h"
#include "subc/token/token.h"

extern file_t *parser_parse_file(char *filename, scope_t *pkg_scope);
