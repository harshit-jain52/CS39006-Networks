all: cldp_client.c cldp_server.c
	gcc -o cldp_client cldp_client.c
	gcc -o cldp_server cldp_server.c -lpthread

clean:
	rm -f cldp_client cldp_server