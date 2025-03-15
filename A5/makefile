all:
	touch qshm
	gcc -o server taskqueueserver.c
	gcc -o client workerclient.c

run: all
	gnome-terminal --title="Server" --geometry=80x24+0+0 -- bash -c "./server; exec bash"
	sleep 1
	gnome-terminal --title="Client" --geometry=50x13+900+0 -- bash -c "./client; exec bash"
	sleep 1
	gnome-terminal --title="Client" --geometry=50x13+900+500 -- bash -c "./client; exec bash"

clean:
	rm -f server client qshm