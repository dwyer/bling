CFLAGS=-I.

main: builtin/builtin.o io/ioutil/ioutil.o kc/token/token.o kc/scanner/scanner.o kc/parser/parser.o emit/emit.o main.o

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

emit/emit.o: builtin/builtin.h kc/ast/ast.h

main.o: builtin/builtin.h kc/ast/ast.h kc/parser/parser.h kc/scanner/scanner.h io/ioutil/ioutil.h
