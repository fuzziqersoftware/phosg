#pragma once

#include <stdint.h>

#include <string>
#include <unordered_map>
#include <vector>


class ConsistentHashRing {
public:
  struct Host {
    std::string name;
    std::string host;
    int port;

    Host(const std::string& descriptor, int default_port);
    Host(const std::string& name, const std::string& host, int port);

    static std::vector<Host> parse_netloc_list(
        const std::vector<std::string>& netlocs, int default_port);

    bool operator==(const Host& other) const;
    bool operator<(const Host& other) const; // for sorting and set<Host>
  };

  virtual ~ConsistentHashRing() = default;

  uint64_t host_id_for_key(const std::string& s) const;
  virtual uint64_t host_id_for_key(const void* key, int64_t size) const = 0;
  const Host& host_for_key(const std::string& s) const;
  const Host& host_for_key(const void* key, int64_t size) const;
  const Host& host_for_id(uint64_t id) const;

  const std::vector<Host>& all_hosts() const;

protected:
  ConsistentHashRing() = default;
  ConsistentHashRing(const std::vector<Host>& hosts);

  std::vector<Host> hosts;
};


class ConstantTimeConsistentHashRing : public ConsistentHashRing {
public:
  ConstantTimeConsistentHashRing() = delete;
  ConstantTimeConsistentHashRing(const std::vector<Host>& hosts,
      uint8_t precision = 17);
  virtual ~ConstantTimeConsistentHashRing() = default;

  virtual uint64_t host_id_for_key(const void* key, int64_t size) const;

  const std::vector<uint8_t>& all_points() const;

protected:
  std::vector<uint8_t> points;
};
