CFLAGS=-fms-extensions -Wno-microsoft-anon-tag

BLINGC=blingc
BUILDFLAGS=

.PHONY: test $(BLINGC) all.bling

hello:
	$(BLINGC) build $(BUILDFLAGS) cmd/blingc
	./gen/cmd/blingc/blingc version

a.out:
	$(BLINGC) -o all.c cmd/blingc/blingc.bling
	$(BLINGC) build cmd/blingc

test_compiler:
	./test_compiler.sh

debug:
	$(CC) -g -I. $(CFLAGS) $(CFILES) \
	    bootstrap/bootstrap.c sys/fmt.c sys/sys.c
	lldb a.out

clean:
	$(RM) -r ./gen

install:
	install ./gen/cmd/blingc/blingc $(HOME)/bin
