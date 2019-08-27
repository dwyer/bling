#pragma once
#include "bootstrap/bootstrap.h"
#include "utils/utils.h"

package(os);

import("paths");
import("utils");

typedef u64 os$Time;

typedef u32 os$FileMode;

typedef struct {
    uintptr fd;
    char *name;
    bool is_dir;
} os$File;

extern os$File *os$stdin;
extern os$File *os$stdout;
extern os$File *os$stderr;

extern const char *os$tempDir();

extern os$File *os$newFile(uintptr fd, const char *name);

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

extern char *os$FileInfo_name(os$FileInfo *info);
extern u64 os$FileInfo_size(os$FileInfo *info);
extern os$FileMode os$FileInfo_mode(os$FileInfo *info);
extern os$Time os$FileInfo_modTime(os$FileInfo *info);
extern bool os$FileInfo_isDir(os$FileInfo *info);
extern void *os$FileInfo_sys(os$FileInfo *info);

extern void os$FileInfo_free(os$FileInfo *info);

extern os$FileInfo *os$stat(const char *filename, utils$Error **error);

extern os$File *os$openDir(const char *filename, utils$Error **error);
extern os$FileInfo **os$readdir(os$File *file, utils$Error **error);

extern void os$mkdirAll(const char *path, u32 mode, utils$Error **error);
