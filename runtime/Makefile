CFLAGS+=
LDFLAGS+=

test: map_test
	./map_test

map_test: map_test.o desc.o map.o slice.o types.o

clean:
	$(RM) map_test *.o
