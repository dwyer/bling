CFLAGS=-I.

BLINGC=bazel-bin/cmd/blingc/blingc

.PHONY: test $(BLINGC) all.bling

SRCS=builtin/builtin.h \
     bootstrap/bootstrap.c \
     error/error.h error/error.c \
     slice/slice.h slice/slice.c \
     map/map.h map/map.c \
     strings/strings.h strings/strings.c \
     path/path.h path/path.c \
     os/os.h os/os.c \
     io/ioutil/ioutil.h io/ioutil/ioutil.c \
     bling/token/token.h bling/token/token.c \
     bling/ast/ast.h bling/ast/ast.c \
     bling/scanner/scanner.h bling/scanner/scanner.c \
     bling/parser/parser.h bling/parser/parser.c \
     bling/emitter/emit.h bling/emitter/emit.c \
     bling/types/types.h bling/types/types.c \
     subc/parser/parser.h subc/parser/parser.c \
     subc/emitter/emit.h subc/emitter/emit.c \
     cmd/blingc/main.c

hello: $(BLINGC) all.bling
	$(BLINGC) -o /dev/stdout -w syntax_test.bling
	$(BLINGC) -o /dev/stdout -w error/error.bling
	$(BLINGC) -o /dev/stdout -w slice/slice.bling

a.out: $(BLINGC) all.bling
	$(BLINGC) -o all.c cmd/blingc/blingc.bling
	cc all.c

test_compiler: $(BLINGC)
	./test_compiler.sh

all.bling: $(BLINGC)
	$(BLINGC) -o all.bling $(SRCS)
	./splitall.py

debug: $(BLINGC)
	lldb $(BLINGC)

$(BLINGC):
	bazel build \
	    --copt="-g0" \
	    --copt="-fms-extensions" \
	    --copt="-Wno-microsoft-anon-tag" \
	    cmd/blingc

clean:
	bazel clean
