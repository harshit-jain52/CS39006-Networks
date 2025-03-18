all: mysmtp_server.c mysmtp_client.c
	gcc -Wall -o mysmtp_server mysmtp_server.c
	gcc -Wall -o mysmtp_client mysmtp_client.c
	mkdir -p mailbox

run: all
	gnome-terminal --title="Server" --geometry=80x24+0+0 -- bash -c "./mysmtp_server 2525; exec bash"
	sleep 1
	gnome-terminal --title="Client" --geometry=50x13+900+0 -- bash -c "./mysmtp_client 127.0.0.1 2525; exec bash"

clean:
	rm -f mysmtp_server mysmtp_client