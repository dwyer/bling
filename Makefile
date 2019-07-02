CFLAGS=-I.

.PHONY: compile test clean

compile:
	./mk-package.py

test: compile
	cat runtime/clib.h >all.c
	./$< runtime/runtime.h \
	    runtime/desc.c runtime/map.c runtime/slice.c runtime/types.c \
	    builtin/builtin.h builtin/builtin.c \
	    io/ioutil/ioutil.h io/ioutil/ioutil.c \
	    subc/token/token.h subc/token/token.c \
	    subc/ast/ast.h subc/ast/ast.c \
	    subc/scanner/scanner.h subc/scanner/scanner.c \
	    subc/parser/parser.h subc/parser/parser.c \
	    subc/emitter/emit.h subc/emitter/emit.c \
	    cmd/compile/main.c \
	    >>all.c
	cc all.c
	./a.out all.c


clean:
	$(RM) compile
	find * -name \*.o -delete
