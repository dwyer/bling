CFLAGS=-I.

.PHONY: compile test clean

compile:
	./mk-package.py

test: compile
	./$< kc/ast/ast.h

clean:
	$(RM) compile
	find * -name \*.o -delete
