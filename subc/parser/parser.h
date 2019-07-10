#pragma once
#include "builtin/builtin.h"
#include "io/ioutil/ioutil.h"
#include "bling/ast/ast.h"
#include "bling/scanner/scanner.h"
#include "bling/token/token.h"
#include "bling/parser/parser.h"

extern file_t *parser_parse_cfile(char *filename, scope_t *pkg_scope);
