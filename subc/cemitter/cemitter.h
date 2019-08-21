#pragma once
#include "bling/emitter/emitter.h"

package(cemitter);

import("bling/ast");
import("bling/emitter");
import("bling/token");
import("sys");
import("utils");

extern void cemitter$emitFile(emitter$Emitter *e, ast$File *file);
extern void cemitter$emitScope(emitter$Emitter *e, ast$Scope *scope);
