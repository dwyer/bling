CFLAGS=-I.

.PHONY: bcc c2c test_bcc test_c2c clean

bcc:
	./mk-package.py

c2c:
	./mk-package.py

test: c2c
	./c2c cmd/c2c/main.c


test_bcc: bcc
	./bcc hello/hello.bling

test_c2c: c2c
	cat runtime/clib.h >all.c
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
	    subc/emitter/emit.h subc/emitter/emit.c \
	    cmd/c2c/main.c \
	    >>all.c
	cc all.c
	./a.out all.c


clean:
	$(RM) c2c
	find * -name \*.o -delete
