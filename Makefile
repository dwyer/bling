CFLAGS=-I.

BLINGC=bazel-bin/cmd/blingc/blingc

.PHONY: test $(BLINGC) all.bling

SRCS=bootstrap/bootstrap.h \
     bootstrap/bootstrap.c \
     fmt/fmt.h \
     error/error.h error/error.c \
     slice/slice.h slice/slice.c \
     map/map.h map/map.c \
     bytes/bytes.h bytes/bytes.c \
     strings/strings.h strings/strings.c \
     path/path.h path/path.c \
     os/os.h \
     io/ioutil/ioutil.h io/ioutil/ioutil.c \
     bling/token/token.h bling/token/token.c \
     bling/ast/ast.h bling/ast/ast.c \
     bling/scanner/scanner.h bling/scanner/scanner.c \
     bling/parser/parser.h bling/parser/parser.c \
     bling/emitter/emitter.h bling/emitter/emitter.c \
     bling/types/types.h bling/types/types.c \
     subc/parser/parser.h subc/parser/parser.c \
     subc/emitter/emitter.h subc/emitter/emitter.c \
     cmd/blingc/main.c

hello: $(BLINGC) all.bling
	$(BLINGC) -o /dev/null -w syntax_test.bling
	$(BLINGC) -o /dev/null -w error/error.bling
	$(BLINGC) -o /dev/null -w slice/slice.bling
	$(BLINGC) -o /dev/null -w map/map.bling
	$(BLINGC) -o /dev/null -w bling/token/token.bling
	$(BLINGC) -o /dev/null -w strings/strings.bling
	$(BLINGC) -o /dev/null -w path/path.bling
	$(BLINGC) -o /dev/null -w bling/ast/ast.bling
	$(BLINGC) -o /dev/null -w bling/scanner/scanner.bling
	$(BLINGC) -o /dev/null -w bling/parser/parser.bling

a.out: $(BLINGC) all.bling
	$(BLINGC) -o all.c cmd/blingc/blingc.bling
	cc bazel-bin/fmt/libfmt.a bazel-bin/os/libos.a all.c

test_compiler: $(BLINGC)
	./test_compiler.sh

all.bling: $(BLINGC)
	$(BLINGC) -c -o all.bling $(SRCS)
	./splitall.py

debug: $(BLINGC)
	lldb $(BLINGC)

$(BLINGC):
	bazel build \
	    -c dbg \
	    --strip=never \
	    --copt="-Wall" \
	    --copt="-Werror" \
	    --copt="-fms-extensions" \
	    --copt="-Wno-microsoft-anon-tag" \
	    cmd/blingc

clean:
	bazel clean
