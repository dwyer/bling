#pragma once
#include "bytes/bytes.h"
#include "os/os.h"

$import("bytes");
$import("os");

extern os_FileInfo **ioutil_read_dir(const char *filename, error_t **error);
extern char *ioutil_read_file(const char *filename, error_t **error);
