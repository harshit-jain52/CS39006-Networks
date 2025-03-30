# Custom Lightweight Discovery Protocol

## Build and Run Instructions

### Files

1. cldp.h
2. cldp_client.c
3. cldp_server.c
4. makefile

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

## Assumptions

1. Maximum size of sending and receiving buffer is `2048 bytes`.
2. A server announces its presence every `10 seconds`.
3. The client queries all servers in its list for metadat every `8 seconds`.
