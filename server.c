#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <errno.h>

/*
 * Command-line arguments
 */
static int port;

static void check_arguments(int argc, char **argv);
static void parse_arguments(int argc, char **argv);

/*
 * Socket-related
 */
#define BUFFER_SIZE    1000     // same as packet size
#define SOCK_NULL 0
static int listenfd = SOCK_NULL;
static int connfd = SOCK_NULL;

static void socket_listen();
static void socket_accept();
static int socket_send_n(char *buf, int n);
static int socket_recv_n(char *buf, int n);

/*
 * Server functionalities
 */
static void process_commands();

int main (int argc, char **argv) {

    check_arguments(argc, argv);
    parse_arguments(argc, argv);

    socket_listen();
    if(listenfd == SOCK_NULL) {
        perror("socket listen fail");
        exit(1);
    }

    while(1) {
        socket_accept();
        if(connfd == SOCK_NULL) {
            perror("socket accept fail");
            continue;
        }

        process_commands();

        close(connfd);
        connfd = SOCK_NULL;
    }

    close(listenfd);
    listenfd = SOCK_NULL;

    return 0;
}

static void process_commands() {
    char buf[4];
    int ret;

    while(1) {
        ret = socket_recv_n(buf, 1);
        if(ret < 0) {
            perror("recv_n");
            exit(1);
        }

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

static void socket_listen() {
    struct sockaddr_in addr;
    int reuseaddr = 1;

    assert(listenfd == SOCK_NULL);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0) {
        perror("socket");
        goto nullify;
    }

    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuseaddr, sizeof(reuseaddr)) < 0) {
        perror("setsockopt(SO_REUSEADDR)");
        goto close;
    }

    bzero((char *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if(bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        goto close;
    }

    if(listen(listenfd, 1) < 0) {
        perror("listen");
        goto close;
    }

    return;

close:
    close(listenfd);
nullify:
    listenfd = SOCK_NULL;
}

static void socket_accept() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    assert(connfd == SOCK_NULL);

    connfd = accept(listenfd, (struct sockaddr *)&addr, &len);
    if(connfd < 0) {
        perror("accept");
        connfd = SOCK_NULL;
    }
}

static int socket_send_n(char *buf, int n) {
    assert(connfd != SOCK_NULL);
    return write(connfd, buf, n);
}

static int socket_recv_n(char *buf, int n) {
    int nleft = n;
    int nread;
    char *bufp = buf;

    assert(connfd != SOCK_NULL);

    while(nleft > 0) {
        if((nread = read(connfd, bufp, nleft)) < 0) {
            if(errno == EINTR)
                nread = 0;
            else
                return -1;
        }else if(nread == 0) {
            break;
        }
        nleft -= nread;
        bufp += nread;
    }
    return (n - nleft);
}
