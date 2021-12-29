#include "ConsistentHashRing.hh"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "Hash.hh"
#include "Strings.hh"

using namespace std;


ConsistentHashRing::Host::Host(const string& descriptor, int default_port) {
  // descriptors are like host[:port][@name]
  // if name isn't given, it defaults to host:port
  size_t at_offset = descriptor.find('@');
  size_t colon_offset = descriptor.find(':');
  if ((colon_offset == string::npos) || (colon_offset > at_offset)) {
    this->host = descriptor.substr(0, at_offset);
    this->port = default_port;
  } else {
    this->host = descriptor.substr(0, colon_offset);
    this->port = stoi(descriptor.substr(colon_offset + 1, at_offset));
  }

  if (at_offset == string::npos) {
    this->name = string_printf("%s:%d", this->host.c_str(), this->port);
  } else {
    this->name = descriptor.substr(at_offset + 1);
  }
}

ConsistentHashRing::Host::Host(const string& name, const string& host, int port)
    : name(name), host(host), port(port) { }

vector<ConsistentHashRing::Host> ConsistentHashRing::Host::parse_netloc_list(
    const vector<string>& netlocs, int default_port) {
  vector<Host> ret;
  for (const string& netloc : netlocs) {
    ret.emplace_back(netloc, default_port);
  }
  return ret;
}

bool ConsistentHashRing::Host::operator==(const Host& other) const {
  return (this->name == other.name) && (this->host == other.host) &&
      (this->port == other.port);
}

bool ConsistentHashRing::Host::operator<(const Host& other) const {
  if (this->name < other.name) {
    return true;
  }
  if (this->name > other.name) {
    return false;
  }

  if (this->host < other.host) {
    return true;
  }
  if (this->host > other.host) {
    return false;
  }

  return this->port < other.port;
}



ConsistentHashRing::ConsistentHashRing(const vector<Host>& hosts) :
    hosts(hosts) { }

uint64_t ConsistentHashRing::host_id_for_key(const string& s) const {
  return this->host_id_for_key(s.data(), s.size());
}

const ConsistentHashRing::Host& ConsistentHashRing::host_for_key(
    const string& s) const {
  return this->host_for_key(s.data(), s.size());
}

const ConsistentHashRing::Host& ConsistentHashRing::host_for_key(
    const void* key, int64_t size) const {
  return this->host_for_id(this->host_id_for_key(key, size));
}

const ConsistentHashRing::Host& ConsistentHashRing::host_for_id(
    uint64_t id) const {
  return this->hosts[id];
}

const vector<ConsistentHashRing::Host>& ConsistentHashRing::all_hosts() const {
  return this->hosts;
}



#define POINTS_PER_HOST 256 // must be divisible by 4

ConstantTimeConsistentHashRing::ConstantTimeConsistentHashRing(
    const vector<Host>& hosts, uint8_t precision) {
  if (hosts.size() > 254) {
    throw runtime_error("too many hosts");
  }

  this->points.clear();
  this->points.resize(1 << precision, 0xFF);

  size_t index_mask = this->points.size() - 1;
  set<Host> hosts_ordered(hosts.begin(), hosts.end());
  for (const auto & it : hosts_ordered) {
    uint8_t host_id = this->hosts.size();
    for (uint16_t y = 0; y < POINTS_PER_HOST; y++) {
      uint64_t h = fnv1a64(it.name.data(), it.name.size(),
          fnv1a64(&y, sizeof(uint16_t)));
      this->points[h & index_mask] = host_id;
    }
    this->hosts.emplace_back(it);
  }

  uint8_t current_host = 0xFF;
  for (size_t index = this->points.size() - 1;
       (index > 0) && (current_host == 0xFF); index--) {
    if (this->points[index] != 0xFF) {
      current_host = this->points[index];
    }
  }
  for (auto& pt : this->points) {
    if (pt == 0xFF) {
      pt = current_host;
    } else {
      current_host = pt;
    }
  }
}

uint64_t ConstantTimeConsistentHashRing::host_id_for_key(const void* key,
    int64_t size) const {
  uint16_t z = 0;
  uint64_t h = fnv1a64(key, size, fnv1a64(&z, sizeof(uint16_t)));
  return this->points[h & (this->points.size() - 1)];
}

const vector<uint8_t>& ConstantTimeConsistentHashRing::all_points() const {
  return this->points;
}
