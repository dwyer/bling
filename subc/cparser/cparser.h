#pragma once
#include "io/ioutil/ioutil.h"
#include "bling/ast/ast.h"
#include "bling/scanner/scanner.h"
#include "bling/token/token.h"
#include "bling/parser/parser.h"

package(cparser);

import("bling/ast");
import("bling/parser");
import("bling/scanner");
import("bling/token");
import("io/ioutil");
import("utils");
import("sys");

extern ast$File *cparser$parseFile(token$FileSet *fset, const char *filename,
        ast$Scope *pkg_scope);
