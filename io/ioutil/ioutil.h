#pragma once
#include "os/os.h"
#include "strings/strings.h"

$import("os");
$import("strings");

extern os_FileInfo **ioutil_read_dir(const char *filename, error_t **error);
extern char *ioutil_read_file(const char *filename, error_t **error);
