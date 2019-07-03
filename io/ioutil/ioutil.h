#pragma once
#include "builtin/builtin.h"
#include "os/os.h"

os_FileInfo **ioutil_read_dir(const char *filename);
char *ioutil_read_file(const char *filename);
