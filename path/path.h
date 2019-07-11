#pragma once
#include "builtin/builtin.h"

extern char *path_base(const char *path);
extern char *path_clean(const char *path);
extern char *path_dir(const char *path);
extern char *path_ext(const char *path);
extern bool path_isAbs(const char *path);
extern char *path_join(int n, ...);
extern char *path_join2(const char *elem1, const char *elem2);
extern char **path_split(const char *path);
