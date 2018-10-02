#pragma once

#include <execinfo.h> // backtrace
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define import(...)
#define package(_)

#define false 0
#define true 1

typedef int bool;

typedef struct {
    void *array;
    int len;
    int cap;
    size_t size;
} slice_t;

extern int len(slice_t s);
extern int cap(slice_t s);
extern void *get_ptr(slice_t s, int index);
extern slice_t append(slice_t s, void *obj);
extern void print(char *fmt, ...);
extern void panic(char *fmt, ...);
