#include <unistd.h>

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Strings.hh"

using namespace std;


bool starts_with(const string& s, const string& start) {
  if (s.length() >= start.length()) {
    return (0 == s.compare(0, start.length(), start));
  }
  return false;
}

bool ends_with(const string& s, const string& end) {
  if (s.length() >= end.length()) {
    return (0 == s.compare(s.length() - end.length(), end.length(), end));
  }
  return false;
}

string string_printf(const char* fmt, ...) {
  va_list va;
  va_start(va, fmt);
  string ret = string_vprintf(fmt, va);
  va_end(va);
  return ret;
}

string string_vprintf(const char* fmt, va_list va) {
  char* result = NULL;
  int length = vasprintf(&result, fmt, va);

  if (result == NULL) {
    throw bad_alloc();
  }

  string ret(result, length);
  free(result);
  return ret;
}

static int current_log_level = INFO;

int log_level() {
  return current_log_level;
}

void set_log_level(int new_level) {
  current_log_level = new_level;
}

static const vector<char> log_level_chars({
  'D', 'I', 'W', 'E',
});

void log(int level, const char* fmt, ...) {
  if (level < current_log_level) {
    return;
  }

  static pid_t pid = 0;

  if (!pid) {
    pid = getpid();
  }

  char time_buffer[32];
  time_t now_secs = time(NULL);
  struct tm now_tm;
  localtime_r(&now_secs, &now_tm);
  strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", &now_tm);
  fprintf(stderr, "%c %d %s - ", log_level_chars[level], pid, time_buffer);

  va_list va;
  va_start(va, fmt);
  vfprintf(stderr, fmt, va);
  va_end(va);
  putc('\n', stderr);
}

vector<string> split(const string& s, char delim) {
  vector<string> elems;
  stringstream ss(s);
  string item;
  while (getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

vector<string> split_context(const string& s, char delim) {
  vector<string> elems;
  vector<char> paren_stack;

  size_t i, last_start = 0;
  for (i = 0; i < s.size(); i++) {
    if (paren_stack.size() > 0) {
      if (s[i] == paren_stack.back())
        paren_stack.pop_back();
    } else {
      if (s[i] == '(')
        paren_stack.push_back(')');
      else if (s[i] == '[')
        paren_stack.push_back(']');
      else if (s[i] == '{')
        paren_stack.push_back('}');
      else if (s[i] == '<')
        paren_stack.push_back('>');
      else if (s[i] == ',') {
        elems.push_back(s.substr(last_start, i - last_start));
        last_start = i + 1;
      }
    }
  }

  if (i >= last_start)
    elems.push_back(s.substr(last_start, i));

  if (paren_stack.size())
    throw runtime_error("Unbalanced parenthesis in split");

  return elems;
}

size_t skip_whitespace(const string& s, size_t offset) {
  while (offset < s.length() &&
      (s[offset] == ' ' || s[offset] == '\t' || s[offset] == '\r' || s[offset] == '\n')) {
    offset++;
  }
  return offset;
}

size_t skip_whitespace(const char* s, size_t offset) {
  while (*s && (s[offset] == ' ' || s[offset] == '\t' || s[offset] == '\r' || s[offset] == '\n')) {
    offset++;
  }
  return offset;
}

size_t skip_non_whitespace(const string& s, size_t offset) {
  while (offset < s.length() &&
      (s[offset] != ' ' && s[offset] != '\t' && s[offset] != '\r' && s[offset] != '\n')) {
    offset++;
  }
  return offset;
}

size_t skip_non_whitespace(const char* s, size_t offset) {
  while (s[offset] && (s[offset] != ' ' && s[offset] != '\t' && s[offset] != '\r' && s[offset] != '\n')) {
    offset++;
  }
  return offset;
}

size_t skip_word(const string& s, size_t offset) {
  return skip_whitespace(s, skip_non_whitespace(s, offset));
}

size_t skip_word(const char* s, size_t offset) {
  return skip_whitespace(s, skip_non_whitespace(s, offset));
}

std::string string_for_error(int error) {
  char buffer[1024];
  strerror_r(error, buffer, sizeof(buffer));
  return string_printf("%d (%s)", error, buffer);
}
