# Edited for Debian GNU/Linux
DESTDIR =

# Where to put binary on 'make install'
BIN = $(DESTDIR)/usr/bin

CFLAGS = -g -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wstrict-prototypes -I./include
OBJECTS = $(BINDIR)/gamelist.o $(BINDIR)/mamecds.o $(BINDIR)/html.o $(BINDIR)/main.o
SRCDIR = src
INCDIR = include
BINDIR = bin


all: $(BINDIR)/mamecds

clean:
	rm -f $(BINDIR)/mamecds $(BINDIR)/*.o

install:
	install $(BINDIR)/mamecds $(BIN)

$(BINDIR)/gamelist.o: $(SRCDIR)/gamelist.c $(INCDIR)/gamelist.h
	gcc ${CFLAGS} -c $(SRCDIR)/gamelist.c -o $(BINDIR)/gamelist.o
	
$(BINDIR)/mamecds.o: $(SRCDIR)/mamecds.c $(INCDIR)/mamecds.h
	gcc ${CFLAGS} -c $(SRCDIR)/mamecds.c -o $(BINDIR)/mamecds.o
	
$(BINDIR)/html.o: $(SRCDIR)/html.c $(INCDIR)/html.h
	gcc ${CFLAGS} -c $(SRCDIR)/html.c -o $(BINDIR)/html.o
	
$(BINDIR)/main.o: $(SRCDIR)/main.c
	gcc ${CFLAGS} -c $(SRCDIR)/main.c -o $(BINDIR)/main.o
	
$(BINDIR)/mamecds: ${OBJECTS}
	gcc ${CFLAGS} ${OBJECTS} -o $(BINDIR)/mamecds
	
