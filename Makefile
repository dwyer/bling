CFLAGS=

BLINGC=blingc
BUILDFLAGS=
BACKUP_DIR=$(HOME)/.bling.bkup
BACKUP_FILE=$(BACKUP_DIR)/blingc-$(shell date +%s)

GEN_DIR=./gen
EXEC=$(GEN_DIR)/cmd/blingc/blingc

.PHONY: $(EXEC) clean debug install

$(EXEC):
	$(BLINGC) build $(BUILDFLAGS) cmd/blingc
	$(EXEC) version

debug:
	$(CC) -g -I. $(CFLAGS) $(shell find bootstrap sys gen -name \*.c)
	lldb a.out

clean:
	$(RM) -r $(GEN_DIR)

install: $(BACKUP_DIR)
	install $(EXEC) $(BACKUP_FILE)
	install $(EXEC) $(HOME)/bin

$(BACKUP_DIR):
	mkdir -p $@
