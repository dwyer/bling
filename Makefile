CFLAGS=-I.

main: builtin/builtin.o io/ioutil/ioutil.o kc/token/token.o kc/scanner/scanner.o kc/parser/parser.o cmd/compile/emit.o cmd/compile/main.o
	cc -o main $+

test: main
	./main kc/ast/ast.h

clean:
	$(RM) main
	find * -name \*.o -delete

builtin/builtin.o: builtin/builtin.h

io/ioutil/ioutil.o: builtin/builtin.h io/ioutil/ioutil.h

kc/token/token.o: builtin/builtin.h kc/token/token.h
kc/scanner/scanner.o: builtin/builtin.h kc/scanner/scanner.h kc/token/token.h
kc/parser/parser.o: builtin/builtin.h kc/ast/ast.h kc/parser/parser.h kc/scanner/scanner.h kc/token/token.h

cmd/compile/emit.o: builtin/builtin.h kc/ast/ast.h

cmd/compile/main.o: builtin/builtin.h kc/ast/ast.h kc/parser/parser.h kc/scanner/scanner.h io/ioutil/ioutil.h
