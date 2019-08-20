CFLAGS=-fms-extensions -Wno-microsoft-anon-tag

BLINGC=bazel-bin/cmd/blingc/blingc

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

hello: $(BLINGC) all.bling
	$(BLINGC) build sys

a.out: $(BLINGC) all.bling
	$(BLINGC) -o all.c cmd/blingc/blingc.bling
	$(CC) $(CFLAGS) all.c \
	    bazel-bin/bootstrap/libbootstrap.a \
	    bazel-bin/os/libos.a bazel-bin/sys/libsys.a

test_compiler: $(BLINGC)
	./test_compiler.sh

all.bling: $(BLINGC)
	$(BLINGC) -c -o all.bling $(SRCS)
	./splitall.py

debug:
	$(CC) -g -I. $(CFLAGS) $(CFILES) \
	    bootstrap/bootstrap.c os/os.c sys/fmt.c sys/sys.c
	lldb a.out

$(BLINGC):
	bazel build \
	    --copt="-Wall" \
	    --copt="-Werror" \
	    --copt="-fms-extensions" \
	    --copt="-Wno-microsoft-anon-tag" \
	    cmd/blingc

clean:
	bazel clean
