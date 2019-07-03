CFLAGS=-I.

.PHONY: b2c test_bcc test_c2c clean

test_c2c: b2c
	./mk-package.py
	./$< runtime/runtime.h \
	    runtime/desc.c runtime/map.c runtime/slice.c runtime/types.c \
	    builtin/builtin.h builtin/builtin.c \
	    os/os.h os/os.c \
	    fmt/fmt.h fmt/fmt.c \
	    io/ioutil/ioutil.h io/ioutil/ioutil.c \
	    subc/token/token.h subc/token/token.c \
	    subc/ast/ast.h subc/ast/ast.c \
	    subc/scanner/scanner.h subc/scanner/scanner.c \
	    subc/parser/parser.h subc/parser/parser.c \
	    bling/emitter/emit.h bling/emitter/emit.c \
	    cmd/b2c/main.c > all.bling
	cat all.bling


clean:
	$(RM) b2c c2c bcc
	find * -name \*.o -delete
