#pragma once

extern void fmt_printf(const char *s, ...);
extern char *fmt_sprintf(const char *s, ...);
extern char *fmt(const char *s, ...); // shorthand for fmt_sprintf
