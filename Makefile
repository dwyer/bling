CFLAGS=-I.

.PHONY: compile test clean

compile:
	./mk-package.py

test: compile
	./$< builtin/builtin.h

clean:
	$(RM) compile
	find * -name \*.o -delete
