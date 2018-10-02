main: main.c slice.c util.c

test: main
	./main

clean:
	$(RM) main *.o

main.c: kc.h util.h
slice.c: kc.h
util.c: kc.h util.h
