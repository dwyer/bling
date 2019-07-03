CFLAGS=-I.

.PHONY: bcc bcc test_bcc test_c2c clean

test_c2c: bcc
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
	    cmd/bcc/main.c


clean:
	$(RM) bcc
	find * -name \*.o -delete
