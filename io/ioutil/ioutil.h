#pragma once
#include "bytes/bytes.h"
#include "os/os.h"

package(ioutil);

import("bytes");
import("utils");
import("os");

extern char *ioutil$readAll(os$File *file, utils$Error **error);
extern os$FileInfo **ioutil$readDir(const char *filename, utils$Error **error);
extern char *ioutil$readFile(const char *filename, utils$Error **error);
extern void ioutil$writeFile(const char *filename, const char *data, int perm,
        utils$Error **error);
