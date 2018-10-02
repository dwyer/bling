CFLAGS=-I.

main: builtin.o io/ioutil/ioutil.o kc/token/token.o kc/scanner/scanner.o kc/parser/parser.o emit/emit.o main.o

test: main
	./main kc/ast/ast.h

clean:
	$(RM) main
	find * -name \*.o -delete

builtin.o: kc.h

io/ioutil/ioutil.o: kc.h io/ioutil/ioutil.h

kc/token/token.o: kc.h kc/token/token.h
kc/scanner/scanner.o: kc.h kc/scanner/scanner.h kc/token/token.h
kc/parser/parser.o: kc.h kc/ast/ast.h kc/parser/parser.h kc/scanner/scanner.h kc/token/token.h

emit/emit.o: kc.h kc/ast/ast.h

main.o: kc.h kc/ast/ast.h kc/parser/parser.h kc/scanner/scanner.h io/ioutil/ioutil.h
