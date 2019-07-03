#pragma once
#include "builtin/builtin.h"

typedef uint64_t time_Time;

typedef uint32_t os_FileMode;

typedef struct {
    char *name;
    uintptr_t fp;
} os_File;

extern os_File *os_open(const char *filename);
extern void os_close(os_File *file);
extern int os_read(os_File *file, char *b, int n);
extern char **os_listdir(const char *dirname);

typedef struct {
    char *name;
    uint64_t size;
    os_FileMode mode;
    time_Time mod_time;
    bool is_dir;
    void *sys;
} os_FileInfo;

extern os_FileInfo os_stat(const char *filename);
