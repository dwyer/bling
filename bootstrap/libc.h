#ifndef __CLIB_H__
#define __CLIB_H__

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#define assert(x) do { if (!(x)) panic("assert failed: " # x); } while (0)

#define esc(x) memcpy(malloc(sizeof(x)), &(x), sizeof(x))

#endif
