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
     subc/parser/parser.h subc/parser/parser.c \
     bling/emitter/emit.h bling/emitter/emit.c \
     cmd/c2bling/main.c

a.out: bazel-bin/cmd/blingc/blingc all.bling
	cat runtime/clib.h >all.c
	bazel-bin/cmd/blingc/blingc all.bling >>all.c
	cc all.c

all.bling: bazel-bin/cmd/c2bling/c2bling
	bazel-bin/cmd/c2bling/c2bling $(SRCS) >all.bling

all.c: bazel-bin/cmd/c2c/c2c
	cat runtime/clib.h >all.c
	bazel-bin/cmd/c2c/c2c $(SRCS) >>all.c

bazel-bin/cmd/blingc/blingc:
	bazel build --copt="-g0" cmd/blingc

bazel-bin/cmd/c2bling/c2bling:
	bazel build --copt="-g0" cmd/c2bling

bazel-bin/cmd/c2c/c2c:
	bazel build --copt="-g0" cmd/c2c


clean:
	bazel clean
