#pragma once

#include <string>

#include <cstdint>

namespace phosg {

uint32_t crc32(const void* vdata, size_t size, uint32_t cs = 0);

constexpr uint32_t FNV1A32_START = 0x811C9DC5;

uint32_t fnv1a32(const void* data, size_t size, uint32_t hash = FNV1A32_START);
uint32_t fnv1a32(const std::string& data, uint32_t hash = FNV1A32_START);

constexpr uint64_t FNV1A64_START = 0xCBF29CE484222325;

uint64_t fnv1a64(const void* data, size_t size, uint64_t hash = FNV1A64_START);
uint64_t fnv1a64(const std::string& data, uint64_t hash = FNV1A64_START);

struct MD5 {
  uint32_t a0, b0, c0, d0;

  MD5(const void* data, size_t size);
  MD5(const std::string& data);

  std::string bin() const;
  std::string hex() const;
};

struct SHA1 {
  uint32_t h[5];

  SHA1(const void* data, size_t size);
  SHA1(const std::string& data);

  std::string bin() const;
  std::string hex() const;
};

struct SHA256 {
  uint32_t h[8];

  SHA256(const void* data, size_t size);
  SHA256(const std::string& data);

  std::string bin() const;
  std::string hex() const;
};

} // namespace phosg
