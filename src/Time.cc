#include "Time.hh"

#include <sys/time.h>
#include <time.h>

#include <format>
#include <stdexcept>
#include <string>

#include "Strings.hh"

using namespace std;

namespace phosg {

uint64_t now() {
  timeval t;
  gettimeofday(&t, nullptr);
  return (uint64_t)t.tv_sec * 1000000 + (uint64_t)t.tv_usec;
}

string format_time(uint64_t t) {
  time_t t_secs = t / 1000000;
  struct tm t_parsed;
#ifndef PHOSG_WINDOWS
  gmtime_r(&t_secs, &t_parsed);
#else
  gmtime_s(&t_parsed, &t_secs);
#endif

  string ret(128, 0);
  size_t len = strftime(ret.data(), ret.size(), "%Y-%m-%d %H:%M:%S", &t_parsed);
  if (len == 0) {
    throw runtime_error("format_time buffer too short");
  }
  int usecs_ret = snprintf(ret.data() + len, ret.size() - len, ".%06" PRIu32, static_cast<uint32_t>(t % 1000000));
  if (usecs_ret < 0) {
    throw runtime_error("cannot add microsecond field");
  }
  ret.resize(min<size_t>(len + usecs_ret, ret.size()));
  return ret;
}

string format_time_natural(uint64_t t) {
  struct timeval tv = usecs_to_timeval(t);
  return format_time_natural(&tv);
}

string format_time_natural(struct timeval* tv) {
  struct timeval local_tv;
  if (!tv) {
    tv = &local_tv;
    gettimeofday(tv, nullptr);
  }

  time_t sec = tv->tv_sec;
  struct tm cooked;
#ifndef PHOSG_WINDOWS
  localtime_r(&sec, &cooked);
#else
  localtime_s(&cooked, &sec);
#endif

  static const char* monthnames[] = {"January", "February", "March", "April",
      "May", "June", "July", "August", "September", "October", "November",
      "December"};

  return std::format("{} {} {:4} {:02}:{:02}:{:02}.{:03}",
      cooked.tm_mday, monthnames[cooked.tm_mon], cooked.tm_year + 1900, cooked.tm_hour, cooked.tm_min, cooked.tm_sec,
      static_cast<uint16_t>(tv->tv_usec / 1000));
}

string format_duration(uint64_t usecs, int8_t subsecond_precision) {
  if (usecs < 1000000ULL) {
    if (subsecond_precision < 0) {
      subsecond_precision = 6;
    }
    return std::format("{:.{}f}", static_cast<double>(usecs) / 1000000ULL, subsecond_precision);

  } else if (usecs < 60 * 1000000ULL) {
    if (subsecond_precision < 0) {
      subsecond_precision = 6;
    }
    return std::format("{:.{}f}", static_cast<double>(usecs) / 1000000ULL, subsecond_precision);

  } else if (usecs < 60 * 60 * 1000000ULL) {
    if (subsecond_precision < 0) {
      subsecond_precision = 3;
    }
    uint64_t minutes = usecs / (60 * 1000000ULL);
    uint64_t usecs_part = usecs - (minutes * 60 * 1000000ULL);
    string seconds_str = std::format("{:.{}f}", static_cast<double>(usecs_part) / 1000000ULL, subsecond_precision);
    return std::format("{}:{}", minutes, ((seconds_str.at(1) == '.') ? "0" : "")) + seconds_str;

  } else if (usecs < 24 * 60 * 60 * 1000000ULL) {
    if (subsecond_precision < 0) {
      subsecond_precision = 0;
    }
    uint64_t hours = usecs / (60 * 60 * 1000000ULL);
    uint64_t minutes = (usecs / (60 * 1000000ULL)) % 60;
    uint64_t usecs_part = usecs - (hours * 60 * 60 * 1000000ULL) - (minutes * 60 * 1000000ULL);
    string seconds_str = std::format("{:.{}f}", static_cast<double>(usecs_part) / 1000000ULL, subsecond_precision);
    return std::format("{}:{:02}:{}", hours, minutes, ((seconds_str.size() == 1 || seconds_str.at(1) == '.') ? "0" : "")) + seconds_str;

  } else {
    if (subsecond_precision < 0) {
      subsecond_precision = 0;
    }
    uint64_t days = usecs / (24 * 60 * 60 * 1000000ULL);
    uint64_t hours = (usecs / (60 * 60 * 1000000ULL)) % 24;
    uint64_t minutes = (usecs / (60 * 1000000ULL)) % 60;
    uint64_t usecs_part = usecs - (days * 24 * 60 * 60 * 1000000ULL) - (hours * 60 * 60 * 1000000ULL) - (minutes * 60 * 1000000ULL);
    string seconds_str = std::format("{:.{}f}", static_cast<double>(usecs_part) / 1000000ULL, subsecond_precision);
    return std::format("{}:{:02}:{:02}:{}", days, hours, minutes, ((seconds_str.size() == 1 || seconds_str.at(1) == '.') ? "0" : "")) + seconds_str;
  }
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

} // namespace phosg
