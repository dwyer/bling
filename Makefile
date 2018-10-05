CFLAGS=-I.

.PHONY: compile test clean

compile:
	./mk-package.py

test: compile
	./$< builtin/builtin.h builtin/builtin.c \
	    kc/token/token.h kc/token/token.c \
	    kc/ast/ast.h \
	    kc/scanner/scanner.h kc/scanner/scanner.c \
	    io/ioutil/ioutil.h io/ioutil/ioutil.c \
	    kc/parser/parser.h kc/parser/parser.c

clean:
	$(RM) compile
	find * -name \*.o -delete
