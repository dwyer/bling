#pragma once
#include "bling/emitter/emitter.h"

package(cemitter);

import("bling/ast");
import("bling/emitter");
import("bling/token");

extern void cemitter$emitFile(emitter$Emitter *e, ast$File *file);