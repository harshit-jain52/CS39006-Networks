SHELL := /bin/sh
CC := gcc
CFLAGS := -g -I. -Wall -L.
RM := rm -f

LIBNAME = libksocket.a 
OBJFILES = ksocket.o

library: $(OBJFILES)
	ar rs $(LIBNAME) $(OBJFILES)

$(OBJFILES): ksocket.h ksocket.c

init: library initksocket.c 
	$(CC) $(CFLAGS) -o initk initksocket.c -lksocket

user: library user1.c user2.c
	$(CC) $(CFLAGS) -o u1 user1.c -lksocket
	$(CC) $(CFLAGS) -o u2 user2.c -lksocket

all: init user

clean:
	-$(RM) $(OBJFILES) initk u1 u2 received*.txt

deepclean: clean
	-$(RM) $(LIBNAME)
