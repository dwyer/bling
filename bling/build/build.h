#pragma once

#include "bootstrap/bootstrap.h"

package(build);

import("bling/ast");
import("bling/emitter");
import("bling/parser");
import("bling/token");
import("bling/types");
import("bytes");
import("io/ioutil");
import("os");
import("paths");
import("subc/cemitter");
import("subc/cparser");
import("sys");
import("utils");

extern void build$buildPackage(char *argv[]);
