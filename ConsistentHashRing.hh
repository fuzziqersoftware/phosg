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

  ConsistentHashRing() = delete;
  ConsistentHashRing(const ConsistentHashRing& other) = default;
  ConsistentHashRing(ConsistentHashRing&& other) = default;
  ConsistentHashRing(const std::vector<Host>& hosts, uint8_t precision = 17);
  virtual ~ConsistentHashRing() = default;

  uint8_t host_id_for_key(const std::string& key) const;
  uint8_t host_id_for_key(const void* key, int64_t size) const;
  const Host& host_for_key(const std::string& key) const;
  const Host& host_for_key(const void* key, int64_t size) const;
  const Host& host_for_id(uint8_t id) const;

  const std::vector<uint8_t>& all_points() const;
  const std::vector<Host>& all_hosts() const;

protected:
  std::vector<uint8_t> points;
  std::vector<Host> hosts;
};
