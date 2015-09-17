#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "socket.h"

/*
 * Command-line arguments
 */
static int port;

static void check_arguments(int argc, char **argv);
static void parse_arguments(int argc, char **argv);

/*
 * Sockets
 */
static int listenfd;
static int connfd;

/*
 * Server functionalities
 */
static void process_commands();

int main (int argc, char **argv) {

    check_arguments(argc, argv);
    parse_arguments(argc, argv);

    listenfd = socket_listen(port);
    assert(listenfd >= 0);

    while(1) {
        connfd = socket_accept(listenfd);
        assert(connfd >= 0);

        process_commands();

        assert(socket_close(connfd) == 0);
    }

    assert(socket_close(listenfd) == 0);

    return 0;
}

static void process_commands() {
    char buf[4];
    int ret;

    while(1) {
        ret = socket_recv_n(connfd, buf, 1);
        assert(ret >= 0);

        if(ret == 0) break;
        switch(buf[0]) {
        case 'W':
            // TODO: set window size
            break;
        case 'R':
            // TODO: response
            break;
        case 'A':
            // TODO: acked
            break;
        default:
            break;
        }
    }
}

static void check_arguments(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }
}

static void parse_arguments(int argc, char **argv) {
    assert(argc == 2);
    port = atoi(argv[1]);
}
