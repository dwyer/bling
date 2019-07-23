#pragma once
#include "strings/strings.h"

import("strings");

extern char *path_base(const char *path);
extern char *path_clean(const char *path);
extern char *path_dir(const char *path);
extern const char *path_ext(const char *path);
extern bool path_isAbs(const char *path);
extern char *path_join(const char **elems, int n);
extern char *path_join2(const char *a, const char *b);
extern char **path_split(const char *path);

extern bool path_match(const char *pattern, const char *path);
extern bool path_matchExt(const char *ext, const char *path);
