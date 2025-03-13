all:
	touch qshm
	gcc -o server taskqueueserver.c
	gcc -o client workerclient.c

clean:
	rm -f server client
	rm -f /dev/shm/sem.qsem qshm
	ipcs -m | grep 777 | awk '{print $2}' | xargs -I {} sh -c 'echo "Removing segment {}"; ipcrm -m {}'