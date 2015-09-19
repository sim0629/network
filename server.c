/* 심규민 2009-11744 */

#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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
static void process_commands(void);
static sem_t window_sem;
static void set_window_size(int window_size);
static void start_file_transfer(int file_number);
static void *transfer_handler(void *param);
static pthread_t transfer_thread;
static void acked(void);

/*
 * File size measuring
 */
static inline off_t get_file_size(const char *file_path) {
    struct stat st;
    if(stat(file_path, &st) == 0)
        return st.st_size;
    return -1;
}

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
            assert(socket_recv_n(connfd, buf, 4) == 4);
            set_window_size(*(int *)buf);
            break;
        case 'R':
            assert(socket_recv_n(connfd, buf, 4) == 4);
            start_file_transfer(*(int *)buf);
            break;
        case 'A':
            acked();
            break;
        default:
            break;
        }
    }
}

static void set_window_size(int window_size) {
    static int initialized = 0;
    if(initialized) {
        assert(sem_destroy(&window_sem) == 0);
    }
    assert(sem_init(&window_sem, 0, window_size) == 0);
    initialized = 1;
}

static void start_file_transfer(int file_number) {
    char *file_names[] = {
        "",
        "TransferMe1.mp4",
        "TransferMe2.mp4",
        "TransferMe3.mp4"
    };

    assert(file_number >= 0 && (unsigned)file_number < sizeof(file_names) / sizeof(char *));

    if(pthread_create(&transfer_thread, NULL, transfer_handler, file_names[file_number]) != 0) {
        fprintf(stderr, "pthread_create failed\n");
        exit(1);
    }
    if(pthread_detach(transfer_thread) != 0) {
        fprintf(stderr, "pthread_detach failed\n");
        exit(1);
    }
}

static void *transfer_handler(void *param) {
    char *file_name = param;
    char file_path[255];
    FILE *fp;
    off_t file_size;

    const size_t packet_size = 1000;
    char buf[packet_size];
    size_t packet_count = 0;
    size_t i;

    assert(strlen(file_name) < 250);
    assert(sprintf(file_path, "data/%s", file_name) >= 0);

    file_size = get_file_size(file_path);
    if(file_size < 0) {
        fprintf(stderr, "The file '%s' not found\n", file_path);
        return NULL;
    }else if(file_size > 0) {
        packet_count = (file_size - 1) / packet_size + 1;
    }

    assert(socket_send_n(connfd, &packet_count, sizeof(size_t)) == sizeof(size_t));

    fp = fopen(file_path, "r");
    for(i = 0; i < packet_count; i++) {
        assert(sem_wait(&window_sem) == 0);
        fread(buf, 1, packet_size, fp);
        assert(socket_send_n(connfd, buf, packet_size) == (ssize_t)packet_size);
    }
    fclose(fp);

    return NULL;
}

static void acked(void) {
    assert(sem_post(&window_sem) == 0);
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
