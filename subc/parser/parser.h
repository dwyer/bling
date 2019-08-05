#pragma once
#include "io/ioutil/ioutil.h"
#include "bling/ast/ast.h"
#include "bling/scanner/scanner.h"
#include "bling/token/token.h"
#include "bling/parser/parser.h"

import("bling/ast");
import("bling/parser");
import("bling/scanner");
import("bling/token");
import("io/ioutil");

extern ast_File *parser_parse_cfile(const char *filename, ast_Scope *pkg_scope);
