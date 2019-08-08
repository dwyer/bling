#ifndef __BOOTSTRAP_H__
#define __BOOTSTRAP_H__

#include <stddef.h>
#include <stdint.h>

#define import(_)
#define package(_)

#define assert(x) do { if (!(x)) panic("assert failed: " # x); } while (0)

#define esc(x) memcpy(malloc(sizeof(x)), &(x), sizeof(x))

typedef char bool;

static const bool false = (bool)0;
static const bool true = !false;

extern void print(const char *s, ...);
extern void panic(const char *s, ...);
extern bool streq(const char *a, const char *b);

extern int memcmp(const void *, const void *, size_t);
extern void *memcpy(void *, const void *, size_t);
extern void *memset(void *, int, size_t);

extern char *strdup(const char *);
extern size_t strlcpy(char *, const char *, size_t);
extern size_t strlen(const char *);

extern void *malloc(size_t);
extern void free(void *);
extern void *realloc(void *, size_t); // used by utils append

#endif
