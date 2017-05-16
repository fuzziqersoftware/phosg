#pragma once

#include <string>

#define fnv1a64_start 0xCBF29CE484222325

uint64_t fnv1a64(const void* data, size_t size, uint64_t hash = fnv1a64_start);
uint64_t fnv1a64(const std::string& data, uint64_t hash = fnv1a64_start);

std::string sha1(const void* data, size_t size);
std::string sha1(const std::string& data);

std::string get_random_data(size_t size);