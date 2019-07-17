CFLAGS=-I.

BLINGC=bazel-bin/cmd/blingc/blingc

.PHONY: test $(BLINGC)

SRCS=runtime/runtime.h \
     runtime/desc.c runtime/map.c runtime/slice.c runtime/types.c \
     builtin/builtin.h builtin/builtin.c \
     strings/strings.h strings/strings.c \
     path/path.h path/path.c \
     os/os.h os/os.c \
     fmt/fmt.h fmt/fmt.c \
     io/ioutil/ioutil.h io/ioutil/ioutil.c \
     bling/token/token.h bling/token/token.c \
     bling/ast/ast.h bling/ast/ast.c \
     bling/scanner/scanner.h bling/scanner/scanner.c \
     bling/parser/parser.h bling/parser/parser.c \
     bling/emitter/emit.h bling/emitter/emit.c \
     subc/parser/parser.h subc/parser/parser.c \
     subc/emitter/emit.h subc/emitter/emit.c \
     cmd/blingc/main.c

hello: $(BLINGC) all.bling
	./splitall.py
	$(BLINGC) -o /dev/null syntax_test.bling

a.out: $(BLINGC) all.bling
	$(BLINGC) -o all.c cmd/blingc/blingc.bling
	cc all.c

all.bling: $(BLINGC)
	$(BLINGC) -o all.bling $(SRCS)

debug: $(BLINGC)
	lldb $(BLINGC)

$(BLINGC):
	bazel build --copt="-g0" cmd/blingc

clean:
	bazel clean
