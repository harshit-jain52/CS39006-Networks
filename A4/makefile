SHELL := /bin/sh
CC := gcc
CFLAGS := -O2 -g -I.
RM := rm -f

LIBNAME = libksocket.a 
OBJFILES = ksocket.o

library: $(OBJFILES)
	ar rs $(LIBNAME) $(OBJFILES)

$(OBJFILES): ksocket.h

clean:
	-$(RM) $(OBJFILES) initk

deepclean:
	-$(RM) $(OBJFILES) initk $(LIBNAME)

run: library initksocket.c
	$(CC) -L. -I. -o initk initksocket.c -lksocket
