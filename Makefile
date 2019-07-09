CFLAGS=-I.

.PHONY: bazel-bin/cmd/c2bling/c2bling bazel-bin/cmd/c2c/c2c test bazel-bin/cmd/blingc/blingc

SRCS=runtime/runtime.h \
     runtime/desc.c runtime/map.c runtime/slice.c runtime/types.c \
     builtin/builtin.h builtin/builtin.c \
     os/os.h os/os.c \
     fmt/fmt.h fmt/fmt.c \
     io/ioutil/ioutil.h io/ioutil/ioutil.c \
     bling/token/token.h bling/token/token.c \
     bling/ast/ast.h bling/ast/ast.c \
     bling/scanner/scanner.h bling/scanner/scanner.c \
     bling/parser/parser.h bling/parser/parser.c \
     subc/emitter/emit.h subc/emitter/emit.c \
     cmd/blingc/main.c

a.out: bazel-bin/cmd/blingc/blingc all.bling
	bazel-bin/cmd/blingc/blingc all.bling >all.c
	cc all.c

all.bling: bazel-bin/cmd/c2bling/c2bling
	bazel-bin/cmd/c2bling/c2bling $(SRCS) >all.bling

all.c: bazel-bin/cmd/c2c/c2c
	bazel-bin/cmd/c2c/c2c $(SRCS) >all.c

bazel-bin/cmd/blingc/blingc:
	bazel build --copt="-g0" cmd/blingc

bazel-bin/cmd/c2bling/c2bling:
	bazel build --copt="-g0" cmd/c2bling

bazel-bin/cmd/c2c/c2c:
	bazel build --copt="-g0" cmd/c2c


clean:
	bazel clean
