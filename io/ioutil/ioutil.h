#pragma once
#include "bytes/bytes.h"
#include "os/os.h"

package(ioutil);

import("bytes");
import("errors");
import("os");

extern char *ioutil$readAll(os$File *file, errors$Error **error);
extern os$FileInfo **ioutil$readDir(const char *filename, errors$Error **error);
extern char *ioutil$readFile(const char *filename, errors$Error **error);
extern void ioutil$writeFile(const char *filename, const char *data, int perm,
        errors$Error **error);
