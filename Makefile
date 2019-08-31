CFLAGS=

BLINGC=blingc
BUILDFLAGS=
BACKUP_DIR=$(HOME)/.bling.bkup
BACKUP_FILE=bling-$(shell date +%s).tar.gz

GEN_DIR=./gen
EXEC=$(GEN_DIR)/cmd/blingc/blingc

.PHONY: $(EXEC) clean debug install

$(EXEC):
	$(BLINGC) build $(BUILDFLAGS) cmd/blingc
	$(EXEC) version

debug:
	$(CC) -g -I. $(CFLAGS) $(shell find C gen sys -name \*.c)
	lldb a.out

clean:
	$(RM) -r $(GEN_DIR)

install: $(BACKUP_DIR)
	tar czf $(BACKUP_FILE) $(GEN_DIR)
	mv $(BACKUP_FILE) $(BACKUP_DIR)
	install $(EXEC) $(HOME)/bin

$(BACKUP_DIR):
	mkdir -p $@
