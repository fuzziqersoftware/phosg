#include "Time.hh"

#include <time.h>
#include <sys/time.h>

#include <stdexcept>
#include <string>

using namespace std;


uint64_t now() {
  timeval t;
  gettimeofday(&t, nullptr);
  return (uint64_t)t.tv_sec * 1000000 + (uint64_t)t.tv_usec;
}

string format_time(uint64_t t) {
  time_t t_secs = t / 1000000;
  struct tm t_parsed;
  gmtime_r(&t_secs, &t_parsed);

  string ret(128, 0);
  size_t len = strftime(ret.data(), ret.size(),
      "%Y-%m-%d %H:%M:%S", &t_parsed);
  if (len == 0) {
    throw runtime_error("format_time buffer too short");
  }
  ret.resize(len);
  return ret;
}

struct timeval usecs_to_timeval(uint64_t usecs) {
  struct timeval tv;
  tv.tv_sec = usecs / 1000000;
  tv.tv_usec = usecs % 1000000;
  return tv;
}

uint64_t timeval_to_usecs(struct timeval& tv) {
  return (tv.tv_sec * 1000000) + tv.tv_usec;
}
