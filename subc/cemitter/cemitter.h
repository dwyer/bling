#pragma once
#include "bling/emitter/emitter.h"
#include "bling/types/types.h"

package(cemitter);

import("bling/ast");
import("bling/emitter");
import("bling/token");
import("bling/types");
import("sys");
import("utils");

extern void cemitter$emitFile(emitter$Emitter *e, ast$File *file);
extern void cemitter$emitScope(emitter$Emitter *e, ast$Scope *scope);
extern void cemitter$emitPackage(emitter$Emitter *e, types$Package *pkg);

extern void cemitter$emitHeader(emitter$Emitter *e, types$Package *pkg);
extern void cemitter$emitBody(emitter$Emitter *e, types$Package *pkg);
