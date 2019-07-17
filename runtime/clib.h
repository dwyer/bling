#ifndef __CLIB_H__
#define __CLIB_H__

#include <sys/stat.h> // os: stat
#include <dirent.h> // os: DIR, dirent, opendir, readdir, closedir
#include <fcntl.h> // os: open
// #include <unistd.h> // os: read, close, write, STD*_FILENO

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <stdio.h>

// #include <stdlib.h>
void *malloc(size_t);
void free(void *);

void *realloc(void *, size_t); // used by slice append

#include <stdbool.h>
#include <errno.h>

#define assert(x) do { if (!(x)) panic("assert failed: " # x); } while (0)

#define esc(x) memcpy(malloc(sizeof(x)), &(x), sizeof(x))

#endif
