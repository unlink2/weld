NAME=weld
IDIR=./include
SDIR=./src 
CC=gcc
DBGCFLAGS=-g -fsanitize=address
DBGLDFLAGS=-fsanitize=address 
CFLAGS=-I$(IDIR) -Wall -pedantic $(DBGCFLAGS)
LIBS=
TEST_LIBS=
LDFLAGS=$(DBGLDFLAGS) $(LIBS)

ODIR=obj
TEST_ODIR=obj/test
BDIR=bin
BNAME=$(NAME)
MAIN=main.o
TEST_MAIN=test.o
TEST_BNAME=testweld

BIN_INSTALL_DIR=/usr/local/bin
MAN_INSTALL_DIR=/usr/local/man

_OBJ = $(MAIN) weld.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

all: bin test

$(ODIR)/%.o: src/%.c
	mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS) $(LDFLAGS)

bin: $(OBJ)
	mkdir -p $(BDIR)
	echo $(BNAME) $(MAIN)
	$(CC) -o $(BDIR)/$(BNAME) $^ $(CFLAGS) $(LDFLAGS)

test:
	echo "building tests"
	make bin MAIN=$(TEST_MAIN) BNAME=$(TEST_BNAME) ODIR=$(TEST_ODIR) LIBS=$(TEST_LIBS)

.PHONY: clean

clean:
	rm -f ./$(ODIR)/*.o
	rm -f ./$(TEST_ODIR)/*.o
	rm -f ./$(BDIR)/$(BNAME)
	rm -f ./$(BDIR)/$(TEST_BNAME)

.PHONY: install 

install:
	cp ./$(BDIR)/$(BNAME) $(BIN_INSTALL_DIR)
	cp ./doc/$(BNAME) $(MAN_INSTALL_DIR)
