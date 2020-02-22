#pragma once

// TODO: a lot of this can be implemented on windows; stop being lazy
#ifndef WINDOWS

#include <stdint.h>
#include <sys/types.h>

#include <string>
#include <utility>


std::pair<struct sockaddr_storage, size_t> make_sockaddr_storage(
    const std::string& addr, int port);

std::string render_sockaddr_storage(const sockaddr_storage& s);

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
 * The port argument may be negative to automatically choose a port (e.g. for
 * UDP sockets where we the caller doesn't need a fixed port number).
 */
int listen(const std::string& addr, int port, int backlog,
    bool nonblocking = true);

/**
 * Connects to a listening socket, possibly on a remote host.
 *
 * If port is nonzero, creates a TCP connection to the host given by addr. If
 * port is zero, creates a Unix socket connection to the given path.
 */
int connect(const std::string& addr, int port, bool nonblocking = true);

void get_socket_addresses(int fd, struct sockaddr_storage* local,
    struct sockaddr_storage* remote);

std::string render_netloc(const std::string& addr, int port);
std::pair<std::string, uint16_t> parse_netloc(const std::string& netloc,
    int default_port = 0);

std::string gethostname();

#endif
