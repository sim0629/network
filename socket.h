#ifndef __SGM_SOCKET_H
#define __SGM_SOCKET_H

#include <inttypes.h>
#include <sys/types.h>

/*
 * Connect to server.
 * Returns the socket descriptor on success, -1 on error.
 */
int socket_connect(const char *hostname, uint16_t port);

/*
 * Listen to port.
 * Returns the socket descriptor on success, -1 on error.
 */
int socket_listen(uint16_t port);

/*
 * Accept a socket from the listening socket.
 * Returns the socket descriptor on success, -1 on error.
 */
int socket_accept(int listenfd);

/*
 * Close a socket.
 * Returns 0 on success, -1 on error.
 */
int socket_close(int sockfd);

/*
 * Send n bytes of data in buffer to the socket.
 * Returns n on success, -1 on error.
 */
ssize_t socket_send_n(int sockfd, const void *buf, size_t n);

/*
 * Receive n bytes of data from the socket to buffer.
 * Returns the number of bytes read on success, -1 on error.
 * It returns less than n only if it meets eof.
 */
ssize_t socket_recv_n(int sockfd, void *buf, size_t n);

#endif
