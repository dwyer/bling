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

typedef struct {
    void *array;
    int len;
    int cap;
    size_t size;
} slice_t;

typedef int bool;

int len(slice_t s);
int cap(slice_t s);
void *get_ptr(slice_t s, int index);
slice_t append(slice_t s, void *obj);
void print(char *fmt, ...);
void panic(char *fmt, ...);
