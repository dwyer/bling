#pragma once
#include "bytes/bytes.h"
#include "os/os.h"

package(ioutil);

import("bytes");
import("os");

extern char *ioutil$readAll(os$File *file, error$Error **error);
extern os$FileInfo **ioutil$readDir(const char *filename, error$Error **error);
extern char *ioutil$readFile(const char *filename, error$Error **error);
extern void ioutil$writeFile(const char *filename, const char *data, int perm,
        error$Error **error);
