#pragma once
#include "io/ioutil/ioutil.h"
#include "bling/ast/ast.h"
#include "bling/scanner/scanner.h"
#include "bling/token/token.h"
#include "bling/parser/parser.h"

$import("bling/ast");
$import("bling/parser");
$import("bling/scanner");
$import("bling/token");
$import("io/ioutil");

extern file_t *parser_parse_cfile(char *filename, scope_t *pkg_scope);
