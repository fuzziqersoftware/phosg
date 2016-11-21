#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <stdexcept>
#include <string>

#include "Filesystem.hh"
#include "Network.hh"
#include "Strings.hh"

using namespace std;


string inet_ntop(const struct sockaddr& sa) {
  if (sa.sa_family == PF_INET) {
    char buffer[INET_ADDRSTRLEN + 1];
    if (!inet_ntop(PF_INET, &((const sockaddr_in*)&sa)->sin_addr, buffer, sizeof(buffer))) {
      string error = string_for_error(errno);
      return string_printf("<INVALID-IPV4-ADDRESS:%s>", error.c_str());
    }
    return string(buffer);
  }

  if (sa.sa_family == PF_INET6) {
    char buffer[INET6_ADDRSTRLEN + 1];
    if (!inet_ntop(PF_INET6, &((const sockaddr_in6*)&sa)->sin6_addr, buffer, sizeof(buffer))) {
      string error = string_for_error(errno);
      return string_printf("<INVALID-IPV6-ADDRESS:%s>", error.c_str());
    }
    return string(buffer);
  }

  return string_printf("<INVALID-ADDRESS-FAMILY:%d>", sa.sa_family);
}

static struct sockaddr_in make_sockaddr_in(const string& addr, int port) {
  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = PF_INET;
  sa.sin_port = htons(port);

  if (addr.empty()) {
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
  } else {
    struct addrinfo *res0, *res;
    if (!getaddrinfo(addr.c_str(), NULL, NULL, &res0)) {
      for (res = res0; res; res = res->ai_next) {
        if (res->ai_family == PF_INET) {
          break;
        }
      }
      if (res) {
        struct sockaddr_in* res_sin = (struct sockaddr_in*)&res->ai_addr;
        sa.sin_addr.s_addr = res_sin->sin_addr.s_addr;
        freeaddrinfo(res0);
      } else {
        freeaddrinfo(res0);
        throw runtime_error("can\'t resolve hostname " + addr + ": no usable data");
      }
    } else {
      throw runtime_error("can\'t resolve hostname " + addr + ": " + string_for_error(errno));
    }
  }
  return sa;
}

static struct sockaddr_un make_sockaddr_un(const string& path) {
  struct sockaddr_un sa;
  if ((path.size() + 1) > sizeof(sa.sun_path)) {
    throw runtime_error("socket path is too long");
  }

  memset(&sa, 0, sizeof(sa));
  sa.sun_family = PF_UNIX;
  strcpy(sa.sun_path, path.c_str());
  return sa;
}

int listen(const string& addr, int port, int backlog, bool nonblocking) {

  int fd = socket(port ? PF_INET : PF_UNIX, backlog ? SOCK_STREAM : SOCK_DGRAM,
      port ? (backlog ? IPPROTO_TCP : IPPROTO_UDP) : 0);
  if (fd == -1) {
    throw runtime_error("can\'t create socket: " + string_for_error(errno));
  }

  int y = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(y)) == -1) {
    close(fd);
    throw runtime_error("can\'t get socket flags: " + string_for_error(errno));
  }

  if (port) {
    // listen on a TCP port
    struct sockaddr_in sa = make_sockaddr_in(addr, port);

    if (::bind(fd, (struct sockaddr*)(&sa), sizeof(sa)) != 0) {
      close(fd);
      throw runtime_error("can\'t bind socket: " + string_for_error(errno));
    }

  } else {
    // listen on a Unix socket
    struct sockaddr_un sa;
    try {
      sa = make_sockaddr_un(addr);
    } catch (const runtime_error& e) {
      close(fd);
      throw;
    }
    unlink(sa.sun_path);

    if (::bind(fd, (struct sockaddr*)(&sa), sizeof(sa)) != 0) {
      close(fd);
      throw runtime_error("can\'t bind socket: " + string_for_error(errno));
    }
  }

  // only listen() on stream sockets
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

  int fd = socket(port ? PF_INET : PF_UNIX, SOCK_STREAM, port ? IPPROTO_TCP : 0);
  if (fd == -1) {
    throw runtime_error("can\'t create socket: " + string_for_error(errno));
  }

  if (port) {
    struct sockaddr_in sa = make_sockaddr_in(addr, port);
    if (connect(fd, (struct sockaddr*)&sa, sizeof(sa)) == -1) {
      close(fd);
      throw runtime_error("can\'t connect socket: " + string_for_error(errno));
    }

  } else {
    struct sockaddr_un sa = make_sockaddr_un(addr);
    if (connect(fd, (struct sockaddr*)&sa, sizeof(sa)) == -1) {
      close(fd);
      throw runtime_error("can\'t connect socket: " + string_for_error(errno));
    }
  }

  if (nonblocking) {
    make_fd_nonblocking(fd);
  }

  return fd;
}

void get_socket_addresses(int fd, struct sockaddr_in* local,
    struct sockaddr_in* remote) {
  socklen_t len;
  if (local) {
    len = sizeof(struct sockaddr_in);
    getsockname(fd, (struct sockaddr*)local, &len);
  }
  if (remote) {
    len = sizeof(struct sockaddr_in);
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

string gethostname() {
  string buf(sysconf(_SC_HOST_NAME_MAX) + 1, 0);
  if (gethostname(const_cast<char*>(buf.data()), buf.size())) {
    throw runtime_error("can\'t get hostname");
  }
  buf.resize(strlen(buf.c_str()));
  return buf;
}
