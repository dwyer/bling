main: builtin.o util.o token.o scanner.o parser.o emit.o main.o

test: main
	./main ast.h

clean:
	$(RM) main *.o

builtin.o: kc.h

util.o: kc.h util.h

token.o: kc.h token.h
scanner.o: kc.h scanner.h token.h
parser.o: kc.h ast.h parser.h scanner.h token.h

emit.o: kc.h ast.h

main.o: kc.h ast.h parser.h scanner.h util.h
