#pragma once

#include <stdint.h>

#include <atomic>
#include <functional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "Encoding.hh"
#include "Strings.hh"
#include "Time.hh"

class CallOnDestroy {
public:
  CallOnDestroy(std::function<void()> f);
  ~CallOnDestroy();

private:
  std::function<void()> f;
};

inline CallOnDestroy on_close_scope(std::function<void()> f) {
  return CallOnDestroy(std::move(f));
}

template <typename IntT>
void parallel_range_thread_fn(
    std::function<bool(IntT, size_t thread_num)>& fn,
    std::atomic<IntT>& current_value,
    std::atomic<IntT>& result_value,
    IntT end_value,
    size_t thread_num) {
  IntT v;
  while ((v = current_value.fetch_add(1)) < end_value) {
    if (fn(v, thread_num)) {
      result_value = v;
      current_value = end_value;
    }
  }
}

template <typename IntT>
void parallel_range_default_progress_fn(IntT start_value, IntT end_value, IntT current_value, uint64_t start_time) {
  std::string format_str = "... %08";
  format_str += printf_hex_format_for_type<IntT>();
  format_str += " (%s / -%s)\r";

  uint64_t elapsed_time = now() - start_time;
  std::string elapsed_str = format_duration(elapsed_time);

  std::string remaining_str;
  if (current_value) {
    uint64_t total_time = (elapsed_time * (end_value - start_value)) / (current_value - start_value);
    uint64_t remaining_time = total_time - elapsed_time;
    remaining_str = format_duration(remaining_time);
  } else {
    remaining_str = "...";
  }

  fprintf(stderr, format_str.c_str(), current_value,
      elapsed_str.c_str(), remaining_str.c_str());
}

// This function runs a function in parallel, using the specified number of
// threads. If the thread count is 0, the function uses the same number of
// threads as there are CPU cores in the system. If any instance of the callback
// returns true, the entire job ends early and all threads stop (after finishing
// their current call to fn, if any). parallel_range returns the value for which
// fn returned true, or it returns end_value if fn never returned true. If
// multiple calls to fn return true, it is not guaranteed which of those values
// is returned (it is often, but not always, the lowest one).
template <typename IntT = uint64_t>
IntT parallel_range(
    std::function<bool(IntT value, size_t thread_num)> fn,
    IntT start_value,
    IntT end_value,
    size_t num_threads = 0,
    std::function<void(IntT start_value, IntT end_value, IntT current_value, uint64_t start_time_usecs)> progress_fn = parallel_range_default_progress_fn<IntT>) {
  if (num_threads == 0) {
    num_threads = std::thread::hardware_concurrency();
  }

  std::atomic<IntT> current_value(start_value);
  std::atomic<IntT> result_value(end_value);
  std::vector<std::thread> threads;
  while (threads.size() < num_threads) {
    threads.emplace_back(
        parallel_range_thread_fn<IntT>,
        std::ref(fn),
        std::ref(current_value),
        std::ref(result_value),
        end_value,
        threads.size());
  }

  if (progress_fn != nullptr) {
    uint64_t start_time = now();
    IntT progress_current_value;
    while ((progress_current_value = current_value.load()) < end_value) {
      progress_fn(start_value, end_value, progress_current_value, start_time);
      usleep(1000000);
    }
  }

  for (auto& t : threads) {
    t.join();
  }

  return result_value;
}
