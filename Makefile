CFLAGS=-I.

.PHONY: compile test clean

compile:
	./mk-package.py

test: compile
	./$< num.c

clean:
	$(RM) compile
	find * -name \*.o -delete
