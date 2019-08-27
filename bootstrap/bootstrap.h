#ifndef BLING_BOOTSTRAP_H
#define BLING_BOOTSTRAP_H

#define import(_) /* noop in C */

#define package(_) /* noop in C */

#define esc(x) ({ \
        void *memcpy(void *, void const*, unsigned long); \
        typeof(x) $0 = (x); \
        memcpy(sys$malloc(sizeof $0), &$0, sizeof $0); \
        })

#define array(T) utils$Array
#define map(T) utils$Map

#define assert(x) do { if (!(x)) panic("assert failed: " # x); } while (0)
#define len(x) (x)._len
#define mapmake(size) {._valSize = size}
#define makemap(T) {._valSize = sizeof(T)}
#define makearray(T) {._size = sizeof(T)}

#define fallthrough /**/
#define typ typedef

#define NULL ((void*)0)

#define false (0)
#define true (!false)

typedef char bool;

typedef char i8;
typedef short i16;
typedef int i32;
typedef long long i64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef u32 uint;
typedef u64 uintptr;

extern void print(const char *s);
extern void _Noreturn panic(const char *s) __attribute__((noreturn));

#endif
