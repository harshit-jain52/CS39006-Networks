# Socket config

0. Protocol-wise

    ```c
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    raw_socket = socket(AF_INET, SOCK_RAW, protocol);
    ```

1. Set timeout for recv

    ```c
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0){
        perror("setsockopt");
        exit(1);
    }
    ```

2. Reuse Address

    ```c
    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("setsockopt");
        exit(1);
    }
    ```

3. Non-blocking

    ```c
    int flags = fcntl(sockfd, F_GETFL, 0);
    if(flags < 0){
        perror("fcntl F_GETFL");
        exit(1);
    }
    if(fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0){
        perror("fcntl F_SETFL");
        exit(1);
    }
    ```

    To get similar behaviour without fcntl():

    ```c
    recv(sockfd, buf, len, MSG_DONTWAIT)
    ```

    ```c
    accept4(sockfd, (struct sockaddr *)&cliaddr, &addr_len, SOCK_NONBLOCK);
    ```

    Errors to handle and not exit from: `EAGAIN` or `EWOULDBLOCK`

4. Broadcast Permission

    ```c
    int opt = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) < 0){
        perror("setsockopt");
        exit(1);
    }
    ```

5. Inform the kernel that the ip header is included in the data

    ```c
    int opt = 1;
    if(setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt)) < 0){
        perror("setsockopt");
        exit(1);
    }
    ```
