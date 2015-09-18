/* 심규민 2009-11744 */

#include "socket.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int socket_connect(const char *hostname, uint16_t port) {
    int sockfd;
    struct sockaddr_in addr;
    struct hostent *server;
    int nodelay = 1;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        perror("socket");
        goto error;
    }

    if(setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&nodelay, sizeof(nodelay)) < 0) {
        perror("setsockopt(TCP_NODELAY)");
        goto error;
    }

    server = gethostbyname(hostname);
    if(server == NULL) {
        perror("gethostbyname");
        goto error;
    }

    bzero((char *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    bcopy((char *)server->h_addr_list[0],
          (char *)&addr.sin_addr.s_addr,
          server->h_length);

    if(connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        goto error;
    }

    return sockfd;

error:
    if(sockfd >= 0) {
        close(sockfd);
    }
    return -1;
}

int socket_listen(uint16_t port) {
    int listenfd;
    struct sockaddr_in addr;
    int reuseaddr = 1;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0) {
        perror("socket");
        goto error;
    }

    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuseaddr, sizeof(reuseaddr)) < 0) {
        perror("setsockopt(SO_REUSEADDR)");
        goto error;
    }

    bzero((char *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if(bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        goto error;
    }

    if(listen(listenfd, 1) < 0) {
        perror("listen");
        goto error;
    }

    return listenfd;

error:
    if(listenfd >= 0) {
        close(listenfd);
    }
    return -1;
}

int socket_accept(int listenfd) {
    int connfd;
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int nodelay = 1;

    connfd = accept(listenfd, (struct sockaddr *)&addr, &len);
    if(connfd < 0) {
        perror("accept");
        goto error;
    }

    if(setsockopt(connfd, IPPROTO_TCP, TCP_NODELAY, (char *)&nodelay, sizeof(nodelay)) < 0) {
        perror("setsockopt(TCP_NODELAY)");
        goto error;
    }

    return connfd;

error:
    if(connfd >= 0) {
        close(connfd);
    }
    return -1;
}

int socket_close(int sockfd) {
    if(close(sockfd) < 0) {
        perror("close socket");
        return -1;
    }
    return 0;
}

ssize_t socket_send_n(int sockfd, const void *buf, size_t n) {
    size_t nleft = n;
    ssize_t nwritten;
    const char *bufp = buf;

    while(nleft > 0) {
        if((nwritten = write(sockfd, buf, n)) <= 0) {
            if(errno == EINTR) {
                nwritten = 0;
            }else {
                perror("write to socket");
                return -1;
            }
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;
}

ssize_t socket_recv_n(int sockfd, void *buf, size_t n) {
    size_t nleft = n;
    ssize_t nread;
    char *bufp = buf;

    while(nleft > 0) {
        if((nread = read(sockfd, bufp, nleft)) < 0) {
            if(errno == EINTR) {
                nread = 0;
            }else {
                perror("read from socket");
                return -1;
            }
        }else if(nread == 0) {
            break;
        }
        nleft -= nread;
        bufp += nread;
    }
    return n - nleft;
}
