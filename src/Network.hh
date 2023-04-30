#pragma once

#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <string>
#include <unordered_map>
#include <utility>

std::pair<struct sockaddr_storage, size_t> make_sockaddr_storage(
    const std::string& addr, uint16_t port);

inline std::pair<struct sockaddr_storage, size_t> make_sockaddr_storage(
    const std::pair<std::string, uint16_t>& netloc) {
  return make_sockaddr_storage(netloc.first, netloc.second);
}

std::string render_sockaddr_storage(const sockaddr_storage& s);

uint32_t resolve_ipv4(const std::string& addr);

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
 *
 * If nonblocking is true, this call (probably) returns before the connection is
 * actually completed. The caller must then use poll(), select(), or some
 * similar syscall to find out when it becomes writable.
 */
int connect(const std::string& addr, int port, bool nonblocking = true);

void get_socket_addresses(int fd, struct sockaddr_storage* local,
    struct sockaddr_storage* remote);

std::string render_netloc(const std::string& addr, int port);
std::pair<std::string, uint16_t> parse_netloc(const std::string& netloc,
    int default_port = 0);

std::string gethostname();

std::pair<int, int> socketpair(
    int domain = AF_LOCAL,
    int type = SOCK_STREAM,
    int protocol = 0);

std::unordered_map<std::string, struct sockaddr_storage> get_network_interfaces();
