#ifndef __CLIB_H__
#define __CLIB_H__

#include <sys/stat.h> // os: stat
#include <dirent.h> // os: DIR, dirent, opendir, readdir, closedir
#include <fcntl.h> // os: open
#include <unistd.h> // os: read, close

#include <execinfo.h> // backtrace
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libgen.h> // basename, dirname

#include <assert.h> // assert
#include <ctype.h> // isspace
#include <time.h> // clock

#include <errno.h>

#endif
