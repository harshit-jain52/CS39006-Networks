# Setup and Running

## Run locally

1. `make runinit` to start initksocket
2. `make runuser` to start 2 user processes \
    `make runuser FOUR_USERS=1` to start 4 user processes

## Run on two different PCs

1. `make runinit` AND `make user` on both PCs
2. on PC-1: `./u1 10.146.xxx.xxx 5050 10.5.xxx.xxx 5051` \
    on PC-2: `./u2 10.5.xxx.xxx 5051 10.146.xxx.xxx 5050`

## Cleanup

1. `make clean` retains the library created
2. `make deepclean`
