#ifndef BLING_BOOTSTRAP_H
#define BLING_BOOTSTRAP_H

#include <stdint.h>

#define import(_) /* noop in C */
#define package(_) /* noop in C */
#define esc(x) ({ \
        void *memcpy(void *, void const*, size_t); \
        typeof(x) $0 = (x); \
        memcpy(malloc(sizeof $0), &$0, sizeof $0); \
        })
#define assert(x) do { if (!(x)) panic("assert failed: " # x); } while (0)

#define fallthrough /**/
#define typ typedef

#define NULL ((void*)0)

#define false (0)
#define true (!false)

typedef unsigned long int size_t;
typedef _Bool bool;

extern void free(void *);
extern void *malloc(size_t);

extern char *strdup(char const *);
extern size_t strlen(char const *);
extern void print(const char *s);
extern void panic(const char *s) __attribute__((noreturn));
extern bool streq(const char *a, const char *b);

#endif
