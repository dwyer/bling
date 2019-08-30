CFLAGS=-fms-extensions -Wno-microsoft-anon-tag

BLINGC=blingc
BUILDFLAGS=

.PHONY: test $(BLINGC) all.bling

SRCS=sys/sys.h \
     utils/utils.h \
     utils/error.c \
     utils/slice.c \
     utils/map.c \
     bytes/bytes.h \
     bytes/bytes.c \
     paths/paths.h \
     paths/paths.c \
     os/os.h \
     os/os.c \
     io/ioutil/ioutil.h \
     io/ioutil/ioutil.c \
     bling/token/token.h \
     bling/token/token.c \
     bling/ast/ast.h \
     bling/ast/ast.c \
     bling/scanner/scanner.h \
     bling/scanner/scanner.c \
     bling/parser/parser.h \
     bling/parser/parser.c \
     bling/emitter/emitter.h \
     bling/emitter/emitter.c \
     bling/types/types.h \
     bling/types/types.c \
     bling/types/universe.c \
     subc/cparser/cparser.h \
     subc/cparser/cparser.c \
     subc/cemitter/cemitter.h \
     subc/cemitter/cemitter.c \
     bling/build/build.h \
     bling/build/build.c \
     cmd/blingc/main.c

CFILES=utils/error.c \
     utils/slice.c \
     utils/map.c \
     bytes/bytes.c \
     paths/paths.c \
     os/os.c \
     io/ioutil/ioutil.c \
     bling/token/token.c \
     bling/ast/ast.c \
     bling/scanner/scanner.c \
     bling/parser/parser.c \
     bling/emitter/emitter.c \
     bling/types/types.c \
     bling/types/universe.c \
     subc/cparser/cparser.c \
     subc/cemitter/cemitter.c \
     bling/build/build.c \
     cmd/blingc/main.c

hello:
	$(BLINGC) build $(BUILDFLAGS) cmd/blingc
	./gen/cmd/blingc/blingc version

a.out:
	$(BLINGC) -o all.c cmd/blingc/blingc.bling
	$(BLINGC) build cmd/blingc

test_compiler:
	./test_compiler.sh

debug:
	$(CC) -g -I. $(CFLAGS) $(CFILES) \
	    bootstrap/bootstrap.c sys/fmt.c sys/sys.c
	lldb a.out

clean:
	$(RM) -r ./gen

install:
	install ./gen/cmd/blingc/blingc $(HOME)/bin
