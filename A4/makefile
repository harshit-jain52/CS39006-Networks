SHELL := /bin/sh
CC := gcc
CFLAGS := -g -I. -Wall -L.
RM := rm -f
IP := 127.0.0.1
PORT1 := 5050
PORT2 := 5051
PORT3 := 8080
PORT4 := 8081
LIBNAME = libksocket.a 
OBJFILES = ksocket.o

FOUR_USERS ?= 0

library: $(OBJFILES)
	ar rs $(LIBNAME) $(OBJFILES)

$(OBJFILES): ksocket.h ksocket.c

init: library initksocket.c 
	$(CC) $(CFLAGS) -o initk initksocket.c -lksocket

user: library user1.c user2.c
	$(CC) $(CFLAGS) -o u1 user1.c -lksocket
	$(CC) $(CFLAGS) -o u2 user2.c -lksocket

runinit: init 
	./initk

runuser: user
	gnome-terminal -- bash -c "./u1 $(IP) $(PORT1) $(IP) $(PORT2); exec bash"
	gnome-terminal -- bash -c "./u2 $(IP) $(PORT2) $(IP) $(PORT1); exec bash"
ifeq ($(FOUR_USERS), 1)
	gnome-terminal -- bash -c "./u1 $(IP) $(PORT3) $(IP) $(PORT4); exec bash"
	gnome-terminal -- bash -c "./u2 $(IP) $(PORT4) $(IP) $(PORT3); exec bash"	
endif

clean:
	-$(RM) $(OBJFILES) initk u1 u2 received*.txt

deepclean: clean
	-$(RM) $(LIBNAME)
