#pragma once
#include "bootstrap/bootstrap.h"
#include "utils/utils.h"

package(os);

import("paths");
import("utils");

typedef uint64_t os$Time;

typedef uint32_t os$FileMode;

typedef struct {
    uintptr_t fd;
    char *name;
    bool is_dir;
} os$File;

extern os$File *os$stdin;
extern os$File *os$stdout;
extern os$File *os$stderr;

extern const char *os$tempDir();

extern os$File *os$newFile(uintptr_t fd, const char *name);

extern os$File *os$openFile(const char *filename, int mode, int perm, utils$Error **error);
extern void os$close(os$File *file, utils$Error **error);

extern os$File *os$open(const char *filename, utils$Error **error);
extern os$File *os$create(const char *filename, utils$Error **error);

extern int os$read(os$File *file, char *b, int n, utils$Error **error);
extern int os$write(os$File *file, const char *b, utils$Error **error);

typedef struct {
    char *_name;
    void *_sys;
} os$FileInfo;

extern char *os$FileInfo_name(os$FileInfo info);
extern uint64_t os$FileInfo_size(os$FileInfo info);
extern os$FileMode os$FileInfo_mode(os$FileInfo info);
extern os$Time os$FileInfo_mod_time(os$FileInfo info);
extern bool os$FileInfo_is_dir(os$FileInfo info);
extern void *os$FileInfo_sys(os$FileInfo info);

extern void os$FileInfo_free(os$FileInfo info);

extern os$FileInfo os$stat(const char *filename, utils$Error **error);

extern os$File *os$openDir(const char *filename, utils$Error **error);
extern os$FileInfo **os$readdir(os$File *file, utils$Error **error);

extern void os$mkdirAll(const char *path, uint32_t mode, utils$Error **error);
