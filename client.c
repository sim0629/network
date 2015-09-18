/* 심규민 2009-11744 */

#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "klist.h"
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
 * ACK-handling thread
 */
static void *ack_handler(void *);
static pthread_t ack_thread;
#define TIMESPEC_FREER(x)
KLIST_INIT(ack, struct timespec, TIMESPEC_FREER);
klist_t(ack) *ack_queue;
pthread_mutex_t ack_queue_lock = PTHREAD_MUTEX_INITIALIZER;

static void start_ack_handling(void);
static void push_ack(void);

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
 * Timespec-related
 */
static inline double timespec_to_seconds(struct timespec *ts) {
    return (double)ts->tv_sec + (double)ts->tv_nsec / 1000000000.0;
}

static inline int timespec_less(struct timespec *lhs, struct timespec *rhs) {
    if(lhs->tv_sec == rhs->tv_sec)
        return lhs->tv_nsec < rhs->tv_nsec;
    return lhs->tv_sec < rhs->tv_sec;
}

static inline void timespec_diff(struct timespec *larger, struct timespec *smaller, struct timespec *diff) {
    diff->tv_sec = larger->tv_sec - smaller->tv_sec;
    diff->tv_nsec = larger->tv_nsec - smaller->tv_nsec;
    if(diff->tv_nsec < 0) {
        diff->tv_sec -= 1;
        diff->tv_nsec += 1000000000;
    }
}

static inline void timespec_add_ms(struct timespec *ts, int ms) {
    ts->tv_nsec += ms * 1000000;
    ts->tv_sec += ts->tv_nsec / 1000000000;
    ts->tv_nsec %= 1000000000;
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

    start_ack_handling();

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

    sleep(1);
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

static void start_ack_handling(void) {
    ack_queue = kl_init(ack);
    if(pthread_create(&ack_thread, NULL, ack_handler, (void *)NULL) != 0) {
        fprintf(stderr, "pthread_create failed\n");
        exit(1);
    }
    if(pthread_detach(ack_thread) != 0) {
        fprintf(stderr, "pthread_detach failed\n");
        exit(1);
    }
}

static void *ack_handler(void *param) {
    struct timespec now;
    struct timespec when;
    int empty;
    char ack[1] = { 'A' };

    while(1) {
        pthread_mutex_lock(&ack_queue_lock);
        empty = kl_shift(ack, ack_queue, &when);
        pthread_mutex_unlock(&ack_queue_lock);

        if(empty) {
            sched_yield();
            continue;
        }

        assert(clock_gettime(CLOCK_MONOTONIC, &now) == 0);
        if(timespec_less(&now, &when)) {
            struct timespec diff;
            timespec_diff(&when, &now, &diff);
            assert(nanosleep(&diff, NULL) == 0);
        }

        assert(socket_send_n(sockfd, ack, 1) == 1);
    }

    return param;
}

static void push_ack() {
    struct timespec ts;

    if(clock_gettime(CLOCK_MONOTONIC, &ts) < 0) {
        perror("getting time in push_ack");
        exit(1);
    }
    timespec_add_ms(&ts, ack_delay_ms);

    pthread_mutex_lock(&ack_queue_lock);
    *kl_pushp(ack, ack_queue) = ts;
    pthread_mutex_unlock(&ack_queue_lock);
}

static size_t sink_packets(size_t packet_count) {
    const size_t packet_size = 1000;
    char buf[packet_size];
    size_t i;

    for(i = 1; i <= packet_count; i++) {
        assert(socket_recv_n(sockfd, buf, packet_size) == (ssize_t)packet_size);

        push_ack();
        if(i % 1000 == 0) {
            printf("%zu MB transmitted\n", i / 1000);
        }
    }

    return packet_size * packet_count;
}
