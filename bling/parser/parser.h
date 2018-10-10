#pragma once
#include "builtin/builtin.h"
#include "io/ioutil/ioutil.h"
#include "bling/ast/ast.h"
#include "bling/scanner/scanner.h"
#include "bling/token/token.h"

extern file_t *parser_parse_file(char *filename, scope_t *pkg_scope);