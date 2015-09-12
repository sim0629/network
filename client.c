#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <signal.h>
#include <assert.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <errno.h>

/*
 * Command-line arguments
 */
static char *hostname;
static int port;
static int window_size;
static int ack_delay_ms;

static void check_arguments(int argc, char **argv);
static void parse_arguments(int argc, char **argv);

/*
 * ACK timer and handler
 */
static void handler();
static timer_t set_timer(long long);
static void set_handler_for_timers();

/*
 * Socket-related
 */
#define BUFFER_SIZE 1000    // same as packet size
#define SOCK_NULL 0
static int sockfd = SOCK_NULL;

static void socket_connect();
static void socket_finish();
static int socket_send_n(char *buf, int n);
static void socket_send_int(char c, int n);
static int socket_recv_n(char *buf, int n);
static int socket_recv_int();
static int socket_recv_packets(int packet_count);

/*
 * Client functionalities
 */
static void command_C();
static void command_R(int file_number);
static void command_F();

int main (int argc, char **argv) {

    check_arguments(argc, argv);
    parse_arguments(argc, argv);

    set_handler_for_timers();

    while(1) {
        char cmd[32];
        if(fgets(cmd, sizeof(cmd), stdin) == NULL) break;

        switch(cmd[0]) {
        case 'C':
            command_C();
            break;
        case 'R':
            command_R(atoi(cmd + 1));
            break;
        case 'F':
            command_F();
            break;
        default:
            break;
        }
    }

    return 0;
}

static void command_C() {
    socket_connect();
    if(sockfd == SOCK_NULL)
        return;
    socket_send_int('W', window_size);
    printf("Connection established\n");
}

static void command_R(int file_number) {
    int packet_count;
    int number_of_bytes;
    socket_send_int('R', file_number);
    packet_count = socket_recv_int();
    number_of_bytes = socket_recv_packets(packet_count);
    printf("File transfer finished\n");
}

static void command_F() {
    socket_finish();
    printf("Connection terminated\n");
}

static void check_arguments(int argc, char **argv) {
    if (argc != 5) {
        printf("Usage: %s <hostname> <port> <window size> <ack delay ms>\n", argv[0]);
        exit(1);
    }
}

static void parse_arguments(int argc, char **argv) {
    assert(argc == 5);
    hostname = argv[1];
    port = atoi(argv[2]);
    window_size = atoi(argv[3]);
    ack_delay_ms = atoi(argv[4]);
}

static void set_handler_for_timers() {
    struct sigaction sigact;
    sigemptyset(&sigact.sa_mask);
    sigaddset(&sigact.sa_mask, SIGALRM);
    sigact.sa_handler = &handler;
    sigaction(SIGALRM, &sigact, NULL);
}

/*
 * handler()
 * Invoked by a timer.
 * Send ACK to the server
 */
static void handler() {
    char buf[1];
    buf[0] = 'A';
    if(socket_send_n(buf, sizeof(buf)) < 0) {
        perror("write to socket in handler");
        exit(1);
    }
}

/*
 * set_timer()
 * set timer in msec
 */
static timer_t set_timer(long long time) {
    struct itimerspec time_spec = {.it_interval = {.tv_sec=0,.tv_nsec=0},
                    .it_value = {.tv_sec=0,.tv_nsec=0}};

    int sec = time / 1000;
    long n_sec = (time % 1000) * 1000;
    time_spec.it_value.tv_sec = sec;
    time_spec.it_value.tv_nsec = n_sec;

    timer_t t_id;
    if (timer_create(CLOCK_MONOTONIC, NULL, &t_id))
        perror("timer_create");
    if (timer_settime(t_id, 0, &time_spec, NULL))
        perror("timer_settime");

    return t_id;
}

static void socket_connect() {
    struct sockaddr_in addr;
    struct hostent *server;
    int nodelay = 1;

    assert(sockfd == SOCK_NULL);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        perror("socket");
        goto nullify;
    }

    if(setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&nodelay, sizeof(nodelay)) < 0) {
        perror("setsockopt(TCP_NODELAY)");
        goto close;
    }

    server = gethostbyname(hostname);
    if(server == NULL) {
        perror("gethostbyname");
        goto close;
    }

    bzero((char *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    bcopy((char *)server->h_addr_list[0],
          (char *)&addr.sin_addr.s_addr,
          server->h_length);

    if(connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        goto close;
    }

    return;

close:
    close(sockfd);
nullify:
    sockfd = SOCK_NULL;
}

static void socket_finish() {
    assert(sockfd != SOCK_NULL);
    close(sockfd);
    sockfd = SOCK_NULL;
}

static int socket_send_n(char *buf, int n) {
    assert(sockfd != SOCK_NULL);
    return write(sockfd, buf, n);
}

static void socket_send_int(char c, int n) {
    char buf[sizeof(n) + 1];
    buf[0] = c;
    *(int *)(buf + 1) = n;
    if(socket_send_n(buf, sizeof(buf)) < 0) {
        perror("write to socket in send_int");
        exit(1);
    }
}

static int socket_recv_n(char *buf, int n) {
    int nleft = n;
    int nread;
    char *bufp = buf;

    assert(sockfd != SOCK_NULL);

    while(nleft > 0) {
        if((nread = read(sockfd, bufp, nleft)) < 0) {
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

static int socket_recv_int() {
    char buf[sizeof(int)];
    if(socket_recv_n(buf, sizeof(buf)) < 0) {
        perror("read from socket in recv_int");
        exit(1);
    }
    return *(int *)buf;
}

static int socket_recv_packets(int packet_count) {
    char buf[BUFFER_SIZE];
    int n_total = 0;
    int i;
    for(i = 1; i <= packet_count; i++) {
        int n = socket_recv_n(buf, sizeof(buf));
        if(n < 0) {
            perror("read from socket in recv_packets");
            exit(1);
        }
        n_total += n;
        set_timer(ack_delay_ms);
        if(i % 10 == 0) {
            printf("%d MB transmitted\n", i / 10);
        }
    }
    return n_total;
}
