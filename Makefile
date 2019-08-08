CFLAGS=-I.

BLINGC=bazel-bin/cmd/blingc/blingc

.PHONY: test $(BLINGC) all.bling

HRDS=bootstrap/bootstrap.h \
     sys/sys.h \
     errors/errors.h \
     utils/utils.h \
     bytes/bytes.h \
     paths/paths.h \
     os/os.h \
     io/ioutil/ioutil.h \
     bling/token/token.h \
     bling/ast/ast.h \
     bling/scanner/scanner.h \
     bling/parser/parser.h \
     bling/emitter/emitter.h \
     bling/types/types.h \
     subc/cparser/cparser.h \
     subc/cemitter/cemitter.h

SRCS=errors/errors.c \
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
     subc/cparser/cparser.c \
     subc/cemitter/cemitter.c \
     cmd/blingc/main.c

a.out: $(BLINGC) all.bling
	$(BLINGC) -o all.c cmd/blingc/blingc.bling
	cc all.c bazel-bin/bootstrap/libbootstrap.a bazel-bin/os/libos.a bazel-bin/sys/libsys.a

hello: $(BLINGC) all.bling
	$(BLINGC) -o /dev/null -w cmd/blingc/blingc.bling

test_compiler: $(BLINGC)
	./test_compiler.sh

all.bling: $(BLINGC)
	$(BLINGC) -c -o all.bling $(HRDS) $(SRCS)
	./splitall.py

debug:
	cc -g -I. $(SRCS) bootstrap/bootstrap.c fmt/fmt.c os/os.c
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
