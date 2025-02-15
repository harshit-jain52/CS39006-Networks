SHELL := /bin/sh
CC := gcc
CFLAGS := -g -I. -Wall -L.
RM := rm -f

LIBNAME = libksocket.a 
OBJFILES = ksocket.o

library: $(OBJFILES)
	ar rs $(LIBNAME) $(OBJFILES)

$(OBJFILES): ksocket.h ksocket.c

clean:
	-$(RM) $(OBJFILES) initk u1 u2 received*.txt

deepclean: clean
	-$(RM) $(LIBNAME)

build: library initksocket.c user1.c user2.c
	$(CC) $(CFLAGS) -o initk initksocket.c -lksocket
	$(CC) $(CFLAGS) -o u1 user1.c -lksocket
	$(CC) $(CFLAGS) -o u2 user2.c -lksocket


