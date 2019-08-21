#ifndef __BOOTSTRAP_H__
#define __BOOTSTRAP_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <strings.h>

#define $
#define import(_)
#define package(_)

#define assert(x) do { if (!(x)) panic("assert failed: " # x); } while (0)

#define esc(x) ({typeof(x) $0 = (x); memcpy(malloc(sizeof $0), &$0, sizeof $0);})

extern void print(const char *s);
extern void panic(const char *s);
extern bool streq(const char *a, const char *b);

#endif
