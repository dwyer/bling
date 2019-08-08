#pragma once
#include "bootstrap/bootstrap.h"

package(path);

import("bytes");

extern char *paths$base(const char *path);
extern char *paths$clean(const char *path);
extern char *paths$dir(const char *path);
extern const char *paths$ext(const char *path);
extern bool paths$isAbs(const char *path);
extern char *paths$join(const char **elems, int n);
extern char *paths$join2(const char *a, const char *b);
extern char **paths$split(const char *path);

extern bool paths$match(const char *pattern, const char *path);
