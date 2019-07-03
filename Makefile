CFLAGS=-I.

.PHONY: bazel-bin/cmd/c2bling/c2bling bazel-bin/cmd/c2c/c2c test

SRCS=runtime/runtime.h \
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
     cmd/c2bling/main.c

test: bazel-bin/cmd/blingc/blingc all.bling
	bazel-bin/cmd/blingc/blingc all.bling




all.bling: bazel-bin/cmd/c2bling/c2bling
	bazel-bin/cmd/c2bling/c2bling $(SRCS) >all.bling

all.c: bazel-bin/cmd/c2c/c2c
	bazel-bin/cmd/c2c/c2c $(SRCS) >all.c

bazel-bin/cmd/blingc/blingc:
	bazel build cmd/blingc

bazel-bin/cmd/c2bling/c2bling:
	bazel build cmd/c2bling

bazel-bin/cmd/c2c/c2c:
	bazel build cmd/c2c


clean:
	bazel clean
