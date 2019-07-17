#pragma once

#define $import(...)
#define $package(_)

#include "runtime/runtime.h"

$import("runtime");

extern void print(const char *fmt, ...);
extern void panic(const char *fmt, ...);
extern bool streq(const char *a, const char *b);
extern void *memdup(const void *src, size_t size);
