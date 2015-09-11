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

int main (int argc, char **argv) {

    check_arguments(argc, argv);
    parse_arguments(argc, argv);

    set_handler_for_timers();

    while(1){}

    // TODO: Create a socket for a client
    //      connect to a server
    //      set ACK delay
    //      set server window size
    //      specify a file to receive
    //      finish the connection

    // TODO: Receive a packet from server
    //      set timer for ACK delay
    //      send ACK packet back to server (usisng handler())
    //      You may use "ack_packet"

    // TODO: Close the socket
    return 0;
}

static void check_arguments(int argc, char **argv) {
    if (argc != 7) {
        printf("Usage: %s <hostname> <port> [Arguments]\n", argv[0]);
        printf("\tMandatory Arguments:\n");
        printf("\t\t-w\t\tsize of the window used at server\n");
        printf("\t\t-d\t\tACK delay in msec\n");
        exit(1);
    }
}

static void parse_arguments(int argc, char **argv) {
    assert(argc == 7);
    hostname = argv[1];
    port = atoi(argv[2]);
    if(strcmp(argv[3], "-w") == 0 && strcmp(argv[5], "-d") == 0) {
        window_size = atoi(argv[4]);
        ack_delay_ms = atoi(argv[6]);
    }else if(strcmp(argv[3], "-d") == 0 && strcmp(argv[5], "-w") == 0) {
        ack_delay_ms = atoi(argv[4]);
        window_size = atoi(argv[6]);
    }else {
        fprintf(stderr, "Error: parsing arguments\n");
        exit(1);
    }
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
