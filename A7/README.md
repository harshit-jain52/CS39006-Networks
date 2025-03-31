# Custom Lightweight Discovery Protocol

## Build and Run Instructions

### Files

1. cldp.h
2. cldp_client.c
3. cldp_server.c
4. makefile
5. cldp_spec.pdf

### Build

Run the following shell command on your device(s):

```shell
make all
```

### Run

1. To run the client:

    ```shell
    sudo ./cldp_client
    ```

2. To run a server:

    ```shell
    sudo ./cldp_server
    ```

> sudo permission is needed to create and manage raw sockets

## Assumptions

1. Maximum size of sending and receiving buffer is `2048 bytes`.
2. A server announces its presence every `10 seconds`.
3. The client queries all servers in its list for metadata every `8 seconds`.
4. The *options* field in the IP header is empty, so the header size is fixed - `20 bytes`.
5. A server is declared inactive if no HELLO message is received from it for `30 seconds`.

## Specifications and Snapshots

Check `cldp_spec.pdf`
