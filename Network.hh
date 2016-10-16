#pragma once

#include <sys/types.h>
#include <sys/socket.h>

#include <string>


std::string inet_ntop(const struct sockaddr& sa);

/**
 * Opens a socket to accept connections or data.
 *
 * This function can open many kinds of sockets:
 * - TCP listening sockets. For these, set addr to the address to listen on, or
 *   "" to listen on all addresses. The port and backlog arguments must be
 *   nonzero.
 * - Unix listening sockets. For these, set addr to the socket's path, and set
 *   port to 0. The backlog must be nonzero.
 * - UDP sockets. For these, set addr to the address to bind to, or "" to bind
 *   to all addresses. The port argument must be nonzero. Set backlog to 0.
 * - Unix datagram sockets. For these, set addr to the socket's path, and set
 *   port and backlog to 0.
 */
int listen(const std::string& addr, int port, int backlog, bool nonblocking = true);

/**
 * Connects to a listening socket, possibly on a remote host.
 *
 * If port is nonzero, creates a TCP connection to the host given by addr. If
 * port is zero, creates a Unix socket connection to the given path.
 */
int connect(const std::string& addr, int port, bool nonblocking = true);

void get_socket_addresses(int fd, struct sockaddr_in* local,
    struct sockaddr_in* remote);

std::string render_netloc(const std::string& addr, int port);
