#pragma once
#include "bootstrap/bootstrap.h"

package(fmt);

extern void fmt$printf(const char *s, ...);
extern char *fmt$sprintf(const char *s, ...);
