#ifndef BLING_BOOTSTRAP_H
#define BLING_BOOTSTRAP_H

#define $$ESC(x) ({ \
        typeof(x) $0 = (x); \
        runtime$memcpy(runtime$malloc(sizeof $0), &$0, sizeof $0); \
        })

#define esc(x) $$ESC(x)

#define array(T) runtime$Slice
#define map(T) runtime$Map

#define append(a, x) do { \
    typeof(x) $0 = (x); \
    runtime$Slice_append(&(a), &$0); \
} while (0)

#define assert(x) do { if (!(x)) print("assert failed: " # x); } while (0)

#define len(x) _Generic((x), \
        runtime$Slice: runtime$Slice_len, \
        runtime$Map: runtime$Map_len)(&(x))

#define makearray(T) (runtime$Slice){.size = sizeof(T)}
#define makemap(T) (runtime$Map){._valSize = sizeof(T)}

#define get(T, a, i) (*(T*)runtime$Slice_get(&(a), (i), nil))

#define NULL ((void*)0)
#define nil ((void*)0)

#define false 0
#define true 1

#define panic(s) runtime$_panic(s)
#define print(s) runtime$_print(s)

typedef char bool;

typedef void *voidptr;
typedef char *charptr;

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

#endif
