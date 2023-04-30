#define __STDC_FORMAT_MACROS

#include "Network.hh"

#include "Platform.hh"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/types.h>
#include <unistd.h>

#include <memory>
#include <stdexcept>
#include <string>

#include "Filesystem.hh"
#include "Strings.hh"

using namespace std;

uint32_t resolve_ipv4(const string& addr) {
  struct addrinfo* res0;
  if (getaddrinfo(addr.c_str(), nullptr, nullptr, &res0)) {
    throw runtime_error("can\'t resolve hostname " + addr + ": " + string_for_error(errno));
  }

  std::unique_ptr<struct addrinfo, void (*)(struct addrinfo*)> res0_unique(
      res0, freeaddrinfo);
  struct addrinfo* res4 = nullptr;
  for (struct addrinfo* res = res0; res; res = res->ai_next) {
    if (!res4 && (res->ai_family == AF_INET)) {
      res4 = res;
    }
  }
  if (!res4) {
    throw runtime_error("can\'t resolve hostname " + addr + ": no usable data");
  }

  struct sockaddr_in* res_sin = (struct sockaddr_in*)res4->ai_addr;
  return ntohl(res_sin->sin_addr.s_addr);
}

pair<struct sockaddr_storage, size_t> make_sockaddr_storage(const string& addr,
    uint16_t port) {
  struct sockaddr_storage s;
  memset(&s, 0, sizeof(s));

  if (port == 0) {
    // unix socket
    struct sockaddr_un* sun = (struct sockaddr_un*)&s;
    if ((addr.size() + 1) > sizeof(sun->sun_path)) {
      throw runtime_error("socket path is too long");
    }

    sun->sun_family = AF_UNIX;
    strcpy(sun->sun_path, addr.c_str());
    return make_pair(s, sizeof(sockaddr_un));
  }

  // inet or inet6

  size_t ret_size = sizeof(struct sockaddr_in);
  if (addr.empty()) {
    struct sockaddr_in* sin = (struct sockaddr_in*)&s;
    sin->sin_family = AF_INET;
    sin->sin_port = (port > 0) ? htons(port) : 0;
    sin->sin_addr.s_addr = htonl(INADDR_ANY);

  } else {
    struct addrinfo* res0;
    if (getaddrinfo(addr.c_str(), nullptr, nullptr, &res0)) {
      throw runtime_error("can\'t resolve hostname " + addr + ": " + string_for_error(errno));
    }

    std::unique_ptr<struct addrinfo, void (*)(struct addrinfo*)> res0_unique(
        res0, freeaddrinfo);
    struct addrinfo *res4 = nullptr, *res6 = nullptr;
    for (struct addrinfo* res = res0; res; res = res->ai_next) {
      if (!res4 && (res->ai_family == AF_INET)) {
        res4 = res;
      } else if (!res6 && (res->ai_family == AF_INET6)) {
        res6 = res;
      }
    }
    if (!res4 && !res6) {
      throw runtime_error("can\'t resolve hostname " + addr + ": no usable data");
    }

    if (res4) {
      struct sockaddr_in* res_sin = (struct sockaddr_in*)res4->ai_addr;
      struct sockaddr_in* sin = (struct sockaddr_in*)&s;
      sin->sin_family = AF_INET;
      sin->sin_port = (port > 0) ? htons(port) : 0;
      sin->sin_addr.s_addr = res_sin->sin_addr.s_addr;

    } else { // res->ai_family == AF_INET6
      struct sockaddr_in6* res_sin6 = (struct sockaddr_in6*)res6->ai_addr;
      struct sockaddr_in6* sin6 = (struct sockaddr_in6*)&s;
      sin6->sin6_family = AF_INET6;
      sin6->sin6_port = (port > 0) ? htons(port) : 0;
      memcpy(&sin6->sin6_addr, &res_sin6->sin6_addr, sizeof(sin6->sin6_addr));

      ret_size = sizeof(struct sockaddr_in6);
    }
  }

  return make_pair(s, ret_size);
}

string render_sockaddr_storage(const sockaddr_storage& s) {
  switch (s.ss_family) {
    case AF_UNIX: {
      struct sockaddr_un* sun = (struct sockaddr_un*)&s;
      return string(sun->sun_path);
    }

    case AF_INET: {
      struct sockaddr_in* sin = (struct sockaddr_in*)&s;

      char buf[INET_ADDRSTRLEN];
      if (!inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf) / sizeof(buf[0]))) {
        return string_printf("<UNPRINTABLE-IPV4-ADDRESS>:%" PRIu16,
            ntohs(sin->sin_port));
      }
      return string_printf("%s:%" PRIu16, buf, ntohs(sin->sin_port));
    }

    case AF_INET6: {
      struct sockaddr_in6* sin6 = (struct sockaddr_in6*)&s;

      char buf[INET6_ADDRSTRLEN];
      if (!inet_ntop(AF_INET6, &sin6->sin6_addr, buf, sizeof(buf) / sizeof(buf[0]))) {
        return string_printf("<UNPRINTABLE-IPV6-ADDRESS>:%" PRIu16,
            ntohs(sin6->sin6_port));
      }
      return string_printf("[%s]:%" PRIu16, buf, ntohs(sin6->sin6_port));
    }

    default:
      return "<INVALID-ADDRESS-FAMILY>";
  }
}

int listen(const string& addr, int port, int backlog, bool nonblocking) {

  pair<struct sockaddr_storage, size_t> s = make_sockaddr_storage(addr, port);

  int fd = socket(s.first.ss_family, backlog ? SOCK_STREAM : SOCK_DGRAM,
      port ? (backlog ? IPPROTO_TCP : IPPROTO_UDP) : 0);
  if (fd == -1) {
    throw runtime_error("can\'t create socket: " + string_for_error(errno));
  }

  int y = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&y), sizeof(y)) == -1) {
    close(fd);
    throw runtime_error("can\'t enable address reuse: " + string_for_error(errno));
  }

  if (port == 0) {
    // Delete the socket file before we start listening on it
    unlink(addr);
  }

  if (::bind(fd, (struct sockaddr*)&s.first, s.second) != 0) {
    close(fd);
    throw runtime_error("can\'t bind socket to " + render_sockaddr_storage(s.first) +
        ": " + string_for_error(errno));
  }

  // New sockets are created with 0755 by default, which is annoying if we're
  // running as root and will drop privileges soon. Make them accessible from
  // everywhere instead.
  if (port == 0) {
    chmod(addr.c_str(), 0777);
  }

  // Only listen() on stream sockets
  if (backlog && (listen(fd, backlog) != 0)) {
    close(fd);
    throw runtime_error("can\'t listen on socket: " + string_for_error(errno));
  }

  if (nonblocking) {
    make_fd_nonblocking(fd);
  }

  return fd;
}

int connect(const string& addr, int port, bool nonblocking) {

  pair<struct sockaddr_storage, size_t> s = make_sockaddr_storage(addr, port);

  int fd = socket(s.first.ss_family, SOCK_STREAM, port ? IPPROTO_TCP : 0);
  if (fd == -1) {
    throw runtime_error("can\'t create socket: " + string_for_error(errno));
  }

  if (nonblocking) {
    make_fd_nonblocking(fd);
  }

  int connect_ret = connect(fd, (struct sockaddr*)&s.first, s.second);
  if (connect_ret == -1 && (!nonblocking || (errno != EINPROGRESS))) {
    close(fd);
    throw runtime_error("can\'t connect socket: " + string_for_error(errno));
  }

  return fd;
}

void get_socket_addresses(int fd, struct sockaddr_storage* local,
    struct sockaddr_storage* remote) {
  socklen_t len;
  if (local) {
    len = sizeof(struct sockaddr_storage);
    getsockname(fd, (struct sockaddr*)local, &len);
  }
  if (remote) {
    len = sizeof(struct sockaddr_storage);
    getpeername(fd, (struct sockaddr*)remote, &len);
  }
}

string render_netloc(const string& addr, int port) {
  if (addr.empty()) {
    if (!port) {
      return "<unknown>";
    } else {
      return to_string(port);
    }
  } else {
    if (!port) {
      return addr;
    } else {
      return addr + ":" + to_string(port);
    }
  }
}

pair<string, uint16_t> parse_netloc(const string& netloc, int default_port) {
  size_t colon_loc = netloc.find(':');
  if (colon_loc == string::npos) {
    return make_pair(netloc, default_port);
  }
  return make_pair(netloc.substr(0, colon_loc), stod(netloc.substr(colon_loc + 1)));
}

string gethostname() {
  string buf(sysconf(_SC_HOST_NAME_MAX) + 1, '\0');
  if (gethostname(buf.data(), buf.size())) {
    throw runtime_error("can\'t get hostname");
  }
  buf.resize(strlen(buf.c_str()));
  return buf;
}

pair<int, int> socketpair(int domain, int type, int protocol) {
  int fds[2];
  if (socketpair(domain, type, protocol, fds)) {
    throw runtime_error("socketpair failed: " + string_for_error(errno));
  }
  return make_pair(fds[0], fds[1]);
}

unordered_map<string, struct sockaddr_storage> get_network_interfaces() {
  struct ifaddrs* ifa_raw;
  if (getifaddrs(&ifa_raw)) {
    throw runtime_error("failed to get interface addresses: " + string_for_error(errno));
  }
  unique_ptr<struct ifaddrs, void (*)(struct ifaddrs*)> ifa(ifa_raw, freeifaddrs);

  unordered_map<string, struct sockaddr_storage> ret;
  for (struct ifaddrs* i = ifa.get(); i; i = i->ifa_next) {
    if (!i->ifa_addr) {
      continue;
    }
    ret.emplace(i->ifa_name, *reinterpret_cast<const struct sockaddr_storage*>(i->ifa_addr));
  }

  return ret;
}
