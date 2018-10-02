CFLAGS=-I.

.PHONY: compile test clean

compile:
	./mk-package.py

test: compile
	./$< builtin/builtin.h builtin/builtin.c kc/token/token.h kc/token/token.c

clean:
	$(RM) compile
	find * -name \*.o -delete
