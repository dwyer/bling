#include "runtime/runtime.h"

extern void printstring(const char *b) {
    fputs(b, stderr);
}

extern void printsp(void) {
    printstring(" ");
}

extern void printnl(void) {
    printstring("\n");
}
