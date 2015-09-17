#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "socket.h"

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
 * Socket
 */
static int sockfd;

/*
 * Message formatting
 */
static inline int make_int_msg(char *buf, char prefix, int n) {
    buf[0] = prefix;
    *(int *)(buf + 1) = n;
    return 1 + sizeof(int);
}

/*
 * Time measuring
 */
static inline double timespec_to_seconds(struct timespec *ts) {
    return (double)ts->tv_sec + (double)ts->tv_nsec / 1000000000.0;
}

/*
 * Client functionalities
 */
static void command_C();
static void command_R(int file_number);
static void command_F();
static size_t sink_packets(size_t packet_count);

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
    char buf[16];
    int n;

    sockfd = socket_connect(hostname, port);
    assert(sockfd >= 0);

    n = make_int_msg(buf, 'W', window_size);
    assert(socket_send_n(sockfd, buf, n) == n);

    printf("Connection established\n");
}

static void command_R(int file_number) {
    char buf[16];
    int n;
    size_t packet_count;
    size_t number_of_bytes;

    n = make_int_msg(buf, 'R', file_number);
    assert(socket_send_n(sockfd, buf, n) == n);

    n = sizeof(size_t);
    assert(socket_recv_n(sockfd, buf, n) == n);
    packet_count = *(size_t *)buf;

    struct timespec start;
    struct timespec end;
    double elapsed_s;
    if(clock_gettime(CLOCK_MONOTONIC, &start)) {
        perror("getting start time");
        exit(1);
    }

    /* Stuff to measure the elapsed time */ {
        number_of_bytes = sink_packets(packet_count);
    }

    if(clock_gettime(CLOCK_MONOTONIC, &end)) {
        perror("getting end time");
        exit(1);
    }
    elapsed_s = timespec_to_seconds(&end) - timespec_to_seconds(&start);

    printf("File transfer finished\n");
    printf("Throughput: %f bps\n", number_of_bytes * 8.0 / elapsed_s);
}

static void command_F() {
    assert(socket_close(sockfd) == 0);
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
    assert(socket_send_n(sockfd, buf, 1) == 1);
}

/*
 * set_timer()
 * set timer in msec
 */
static timer_t set_timer(long long time) {
    struct itimerspec time_spec = {
        .it_interval = {
            .tv_sec = 0,
            .tv_nsec = 0,
        },
        .it_value = {
            .tv_sec = 0,
            .tv_nsec = 0,
        }
    };

    int sec = time / 1000;
    long n_sec = (time % 1000) * 1000;
    time_spec.it_value.tv_sec = sec;
    time_spec.it_value.tv_nsec = n_sec;

    timer_t t_id;
    if(timer_create(CLOCK_MONOTONIC, NULL, &t_id)) {
        perror("timer_create");
        exit(1);
    }
    if(timer_settime(t_id, 0, &time_spec, NULL)) {
        perror("timer_settime");
        exit(1);
    }

    return t_id;
}

static size_t sink_packets(size_t packet_count) {
    const size_t packet_size = 1000;
    char buf[packet_size];
    size_t i;

    for(i = 1; i <= packet_count; i++) {
        assert(socket_recv_n(sockfd, buf, packet_size) == (ssize_t)packet_size);

        set_timer(ack_delay_ms);
        if(i % 10 == 0) {
            printf("%zu MB transmitted\n", i / 10);
        }
    }

    return packet_size * packet_count;
}
