#include "Time.hh"

#include <sys/time.h>
#include <time.h>

#include <stdexcept>
#include <string>

#include "Strings.hh"

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

string format_time(struct timeval* tv) {
  struct timeval local_tv;
  if (!tv) {
    tv = &local_tv;
    gettimeofday(tv, nullptr);
  }

  time_t sec = tv->tv_sec;
  struct tm cooked;
  localtime_r(&sec, &cooked);

  static const char* monthnames[] = {"January", "February", "March", "April",
      "May", "June", "July", "August", "September", "October", "November",
      "December"};

  return string_printf("%u %s %4u %02u:%02u:%02u.%03hu", cooked.tm_mday,
      monthnames[cooked.tm_mon], cooked.tm_year + 1900,
      cooked.tm_hour, cooked.tm_min, cooked.tm_sec,
      static_cast<uint16_t>(tv->tv_usec / 1000));
}

string format_duration(uint64_t usecs, int8_t subsecond_precision) {
  if (usecs < 1000000ULL) {
    if (subsecond_precision < 0) {
      subsecond_precision = 5;
    }
    return string_printf("%.*lf", subsecond_precision,
        static_cast<double>(usecs) / 1000000ULL);

  } else if (usecs < 60 * 1000000ULL) {
    if (subsecond_precision < 0) {
      subsecond_precision = (usecs < 10 * 1000000ULL) ? 5 : 4;
    }
    return string_printf("%.*lf", subsecond_precision,
        static_cast<double>(usecs) / 1000000ULL);

  } else if (usecs < 60 * 60 * 1000000ULL) {
    if (subsecond_precision < 0) {
      subsecond_precision = (usecs < 10 * 60 * 1000000ULL) ? 2 : 1;
    }
    uint64_t minutes = usecs / (60 * 1000000ULL);
    uint64_t usecs_part = usecs - (minutes * 60 * 1000000ULL);
    string seconds_str = string_printf("%.*lf", subsecond_precision,
        static_cast<double>(usecs_part) / 1000000ULL);
    return string_printf("%" PRIu64 ":%s", minutes,
               ((seconds_str.at(1) == '.') ? "0" : "")) +
        seconds_str;

  } else if (usecs < 24 * 60 * 60 * 1000000ULL) {
    if (subsecond_precision < 0) {
      subsecond_precision = 0;
    }
    uint64_t hours = usecs / (60 * 60 * 1000000ULL);
    uint64_t minutes = (usecs / (60 * 1000000ULL)) % 60;
    uint64_t usecs_part = usecs - (hours * 60 * 60 * 1000000ULL) - (minutes * 60 * 1000000ULL);
    string seconds_str = string_printf("%.*lf", subsecond_precision,
        static_cast<double>(usecs_part) / 1000000ULL);
    return string_printf("%" PRIu64 ":%02" PRIu64 ":%s", hours, minutes,
               ((seconds_str.size() == 1 || seconds_str.at(1) == '.') ? "0" : "")) +
        seconds_str;

  } else {
    if (subsecond_precision < 0) {
      subsecond_precision = 0;
    }
    uint64_t days = usecs / (24 * 60 * 60 * 1000000ULL);
    uint64_t hours = (usecs / (60 * 60 * 1000000ULL)) % 24;
    uint64_t minutes = (usecs / (60 * 1000000ULL)) % 60;
    uint64_t usecs_part = usecs - (days * 24 * 60 * 60 * 1000000ULL) - (hours * 60 * 60 * 1000000ULL) - (minutes * 60 * 1000000ULL);
    string seconds_str = string_printf("%.*lf", subsecond_precision,
        static_cast<double>(usecs_part) / 1000000ULL);
    return string_printf("%" PRIu64 ":%02" PRIu64 ":%02" PRIu64 ":%s", days,
               hours, minutes, ((seconds_str.size() == 1 || seconds_str.at(1) == '.') ? "0" : "")) +
        seconds_str;
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
