#pragma once

#include <string>

#include <cstdint>

namespace phosg {

constexpr uint32_t FNV1A32_START = 0x811C9DC5;

uint32_t fnv1a32(const void* data, size_t size, uint32_t hash = FNV1A32_START);
uint32_t fnv1a32(const std::string& data, uint32_t hash = FNV1A32_START);

constexpr uint64_t FNV1A64_START = 0xCBF29CE484222325;

uint64_t fnv1a64(const void* data, size_t size, uint64_t hash = FNV1A64_START);
uint64_t fnv1a64(const std::string& data, uint64_t hash = FNV1A64_START);

std::string sha1(const void* data, size_t size);
std::string sha1(const std::string& data);

std::string sha256(const void* data, size_t size);
std::string sha256(const std::string& data);

std::string md5(const void* data, size_t size);
std::string md5(const std::string& data);

uint32_t crc32(const void* vdata, size_t size, uint32_t cs = 0);

} // namespace phosg
