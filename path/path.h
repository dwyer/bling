#pragma once
#include "bootstrap/bootstrap.h"

package(path);

import("bytes");

extern char *path$base(const char *path);
extern char *path$clean(const char *path);
extern char *path$dir(const char *path);
extern const char *path$ext(const char *path);
extern bool path$isAbs(const char *path);
extern char *path$join(const char **elems, int n);
extern char *path$join2(const char *a, const char *b);
extern char **path$split(const char *path);

extern bool path$match(const char *pattern, const char *path);
