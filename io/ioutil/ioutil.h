#pragma once
#include "os/os.h"

$import("fmt");
$import("os");

os_FileInfo **ioutil_read_dir(const char *filename);
char *ioutil_read_file(const char *filename);
