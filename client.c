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

#define BUFFER_SIZE 1000    // same as packet size

// Command-line arguments
static char *hostname;
static int port;
static int window_size;
static int ack_delay_ms;

static void check_arguments(int argc, char **argv);
static void parse_arguments(int argc, char **argv);

// ACK timer and handler
static void handler();
static timer_t set_timer(long long);
static void set_handler_for_timers();

// Socket
#define SOCK_NULL 0
static int sockfd = SOCK_NULL;

static void socket_connect();
static void socket_finish();

int main (int argc, char **argv) {

    check_arguments(argc, argv);
    parse_arguments(argc, argv);

    set_handler_for_timers();

    while(1) {
        char cmd[32];
        if(fgets(cmd, sizeof(cmd), stdin) == NULL) break;

        switch(cmd[0]) {
        case 'C':
            socket_connect();
            break;
        case 'R':
            // TODO
            break;
        case 'F':
            socket_finish();
            break;
        default:
            break;
        }
    }

    return 0;
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
    printf("Hi\n");
    // TODO: Send an ACK packet
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
