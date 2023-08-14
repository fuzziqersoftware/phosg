#pragma once

#include <string>

#include <stdint.h>

#define fnv1a32_start 0x811C9DC5

uint32_t fnv1a32(const void* data, size_t size, uint32_t hash = fnv1a32_start);
uint32_t fnv1a32(const std::string& data, uint32_t hash = fnv1a32_start);

#define fnv1a64_start 0xCBF29CE484222325

uint64_t fnv1a64(const void* data, size_t size, uint64_t hash = fnv1a64_start);
uint64_t fnv1a64(const std::string& data, uint64_t hash = fnv1a64_start);

std::string sha1(const void* data, size_t size);
std::string sha1(const std::string& data);

std::string sha256(const void* data, size_t size);
std::string sha256(const std::string& data);

std::string md5(const void* data, size_t size);
std::string md5(const std::string& data);

uint32_t crc32(const void* vdata, size_t size, uint32_t cs = 0);
