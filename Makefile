CFLAGS=-fms-extensions -Wno-microsoft-anon-tag

BLINGC=blingc
BUILDFLAGS=
BACKUP_DIR=$(HOME)/.bling.bkup
BACKUP_FILE=$(BACKUP_DIR)/blingc-$(shell date +%s)

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
	$(CC) -g -I. $(CFLAGS) $(shell find bootstrap sys gen -name \*.c)
	lldb a.out

clean:
	$(RM) -r ./gen

install: $(BACKUP_DIR)
	install ./gen/cmd/blingc/blingc $(BACKUP_FILE)
	install ./gen/cmd/blingc/blingc $(HOME)/bin

$(BACKUP_DIR):
	mkdir -p $@
