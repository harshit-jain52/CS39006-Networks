SHELL := /bin/sh
CC := gcc
CFLAGS := -O2 -g -I. -Wall
RM := rm -f

LIBNAME = libksocket.a 
OBJFILES = ksocket.o

library: $(OBJFILES)
	ar rs $(LIBNAME) $(OBJFILES)

$(OBJFILES): ksocket.h ksocket.c

clean:
	-$(RM) $(OBJFILES) initk

deepclean:
	-$(RM) $(OBJFILES) initk $(LIBNAME)

build: library initksocket.c
	$(CC) -Wall -L. -I. -o initk initksocket.c -lksocket
