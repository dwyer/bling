#pragma once
#include "bling/emitter/emitter.h"

package(cemitter);

import("bling/emitter");

extern void cemitter$emitFile(emitter$Emitter *e, ast$File *file);
