#pragma once
#include "bytes/bytes.h"
#include "os/os.h"

import("bytes");
import("os");

extern char *ioutil_readAll(os$File *file, error_t **error);
extern os$FileInfo **ioutil_readDir(const char *filename, error_t **error);
extern char *ioutil_readFile(const char *filename, error_t **error);
extern void ioutil_writeFile(const char *filename, const char *data, int perm,
        error_t **error);
