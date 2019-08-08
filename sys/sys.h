#pragma once
#include "bootstrap/bootstrap.h"

package(sys);

extern void sys$printf(const char *s, ...);
extern char *sys$sprintf(const char *s, ...);
