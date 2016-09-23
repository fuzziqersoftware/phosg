#pragma once

#include <stdint.h>
#include <sys/types.h>

#include <string>
#include <vector>


bool starts_with(const std::string& s, const std::string& start);
bool ends_with(const std::string& s, const std::string& end);

std::string string_printf(const char* fmt, ...);
std::string string_vprintf(const char* fmt, va_list va);

#define DEBUG 0
#define INFO 1
#define WARNING 2
#define ERROR 3

int log_level();
void set_log_level(int new_level);
void log(int level, const char* fmt, ...);

std::vector<std::string> split(const std::string& s, char delim);
std::vector<std::string> split_context(const std::string& s, char delim);

size_t skip_whitespace(const std::string& s, size_t offset);
size_t skip_whitespace(const char* s, size_t offset);
size_t skip_non_whitespace(const std::string& s, size_t offset);
size_t skip_non_whitespace(const char* s, size_t offset);
size_t skip_word(const std::string& s, size_t offset);
size_t skip_word(const char* s, size_t offset);

std::string string_for_error(int error);
