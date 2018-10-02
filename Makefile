CFLAGS=-I.

.PHONY: compile test clean

compile:
	./mk-package.py

test: compile
	./$< builtin/builtin.h builtin/builtin.c

clean:
	$(RM) compile
	find * -name \*.o -delete
