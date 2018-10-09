CFLAGS=-I.

.PHONY: compile test clean

compile:
	./mk-package.py

test: compile
	./$< runtime/runtime.h \
	    builtin/builtin.h builtin/builtin.cc \
	    io/ioutil/ioutil.h io/ioutil/ioutil.cc \
	    kc/token/token.h kc/token/token.cc \
	    kc/ast/ast.h \
	    kc/scanner/scanner.h kc/scanner/scanner.cc \
	    kc/parser/parser.h kc/parser/parser.cc

clean:
	$(RM) compile
	find * -name \*.o -delete
