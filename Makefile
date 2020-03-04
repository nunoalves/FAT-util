CC=gcc
LDIR=src/libs
ODIR=src/objs
CFLAGS=-Wall -I$(LDIR)

$(ODIR)/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: clean fat-util

fat-util: $(ODIR)/main.o $(ODIR)/fat.o $(ODIR)/commands.o
	${CC} $(CFLAGS) -o fat-util $(ODIR)/*.o

clean:
	rm -f $(ODIR)/*.o *~ fat-util 