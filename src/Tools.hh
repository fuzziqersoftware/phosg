#pragma once

#include <stdint.h>

#include <atomic>
#include <format>
#include <functional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "Encoding.hh"
#include "Filesystem.hh"
#include "Strings.hh"
#include "Time.hh"

namespace phosg {

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
void parallel_noop_progress_fn(IntT, IntT, IntT, uint64_t) {
}

template <typename IntT>
void parallel_default_progress_fn(IntT start_value, IntT end_value, IntT current_value, uint64_t start_time) {
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

  fwritex(stderr, std::format("... {:08X} ({} / {})\r", current_value, elapsed_str, remaining_str));
}

template <typename IntT, typename FnT>
  requires(std::is_invocable_r_v<bool, FnT, IntT, size_t>)
void parallel_thread_fn(
    FnT& fn, std::atomic<IntT>& current_value, std::atomic<IntT>& result_value, IntT end_value, size_t thread_num) {
  IntT v;
  while ((v = current_value.fetch_add(1)) < end_value) {
    if (fn(v, thread_num)) {
      result_value = v;
      current_value = end_value;
    }
  }
}

// This function runs a function in parallel, using the specified number of threads. If the thread count is 0, the
// function uses the same number of threads as there are CPU cores in the system. If any instance of the callback
// returns true, the entire job ends early and all threads stop (after finishing their current call to fn, if any).
// parallel returns the value for which fn returned true, or it returns end_value if fn never returned true. If
// multiple calls to fn return true, it is not guaranteed which of those values is returned (it is often, but not
// always, the lowest one).
template <typename IntT = uint64_t, typename FnT, typename ProgressFnT = void (*)(IntT, IntT, IntT, uint64_t)>
  requires(
      std::is_invocable_r_v<bool, FnT, IntT, size_t> &&
      std::is_invocable_v<ProgressFnT, IntT, IntT, IntT, uint64_t>)
IntT parallel(
    FnT&& fn,
    IntT start_value,
    IntT end_value,
    size_t num_threads = 0,
    ProgressFnT&& progress_fn = parallel_default_progress_fn<IntT>,
    bool use_progress_fn = true) {
  if (num_threads == 0) {
    num_threads = std::thread::hardware_concurrency();
  }

  std::atomic<IntT> current_value(start_value);
  std::atomic<IntT> result_value(end_value);
  std::vector<std::thread> threads;
  while (threads.size() < num_threads) {
    threads.emplace_back(
        parallel_thread_fn<IntT, FnT>,
        std::ref(fn),
        std::ref(current_value),
        std::ref(result_value),
        end_value,
        threads.size());
  }

  if (use_progress_fn) {
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

// This is a bit of a hack, but it allows us to implement a shorthand for suppressing the default status reporting by
// passing nullptr for the status function (even though it's not a callable according to the `requires` condition).
template <typename IntT = uint64_t, typename FnT>
  requires(std::is_invocable_r_v<bool, FnT, IntT, size_t>)
IntT parallel(FnT&& fn, IntT start_value, IntT end_value, size_t num_threads, std::nullptr_t) {
  return parallel<IntT, FnT>(
      std::forward<FnT>(fn), start_value, end_value, num_threads, parallel_noop_progress_fn<IntT>, false);
}

template <typename IntT, typename FnT>
  requires(std::is_invocable_r_v<bool, FnT, IntT, size_t>)
void parallel_blocks_thread_fn(
    FnT& fn,
    std::atomic<IntT>& current_value,
    std::atomic<IntT>& result_value,
    IntT end_value,
    IntT block_size,
    size_t thread_num) {
  IntT block_start;
  while ((block_start = current_value.fetch_add(block_size)) < end_value) {
    IntT block_end = block_start + block_size;
    for (IntT z = block_start; z < block_end; z++) {
      if (fn(z, thread_num)) {
        result_value = z;
        current_value = end_value;
        break;
      }
    }
  }
}

// Like parallel, but faster due to fewer atomic memory operations. block_size must evenly divide the input value
// range, but it is not required that start_value or end_value themselves be multiples of block_size. For example, the
// following are both valid inputs:
//   start_value = 0x00000000, end_value = 0x10000000, block_size = 0x1000
//   start_value = 0x47F92AC2, end_value = 0x67F92AC2, block_size = 0x10000
// However, this would be invalid:
//   start_value = 0x0000004F, end_value = 0x10000059, block_size = 0x10000
template <typename IntT = uint64_t, typename FnT, typename ProgressFnT = void (*)(IntT, IntT, IntT, uint64_t)>
  requires(
      std::is_invocable_r_v<bool, FnT, IntT, size_t> &&
      std::is_invocable_v<ProgressFnT, IntT, IntT, IntT, uint64_t>)
IntT parallel_blocks(
    FnT&& fn,
    IntT start_value,
    IntT end_value,
    IntT block_size,
    size_t num_threads = 0,
    ProgressFnT&& progress_fn = parallel_default_progress_fn<IntT>,
    bool use_progress_fn = true) {
  if ((end_value - start_value) % block_size) {
    throw std::logic_error("block_size must evenly divide the entire range");
  }

  if (num_threads == 0) {
    num_threads = std::thread::hardware_concurrency();
  }
  if (num_threads < 1) {
    throw std::logic_error("thread count must be at least 1");
  }

  std::atomic<IntT> current_value(start_value);
  std::atomic<IntT> result_value(end_value);
  std::vector<std::thread> threads;
  while (threads.size() < num_threads) {
    threads.emplace_back(
        parallel_blocks_thread_fn<IntT, FnT>,
        std::ref(fn),
        std::ref(current_value),
        std::ref(result_value),
        end_value,
        block_size,
        threads.size());
  }

  if (use_progress_fn) {
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

template <typename IntT = uint64_t, typename FnT>
  requires(std::is_invocable_r_v<bool, FnT, IntT, size_t>)
IntT parallel_blocks(
    FnT&& fn, IntT start_value, IntT end_value, IntT block_size, size_t num_threads, std::nullptr_t) {
  return parallel_blocks<IntT, FnT>(
      std::forward<FnT>(fn), start_value, end_value, block_size, num_threads, parallel_noop_progress_fn<IntT>, false);
}

// Like parallel_blocks, but returns all values for which fn returned true. (Unlike the other parallel functions, this
// one does not return early.)
template <typename IntT = uint64_t, typename RetT = std::unordered_set<IntT>, typename FnT, typename ProgressFnT = void (*)(IntT, IntT, IntT, uint64_t)>
  requires(
      std::is_invocable_r_v<bool, FnT, IntT, size_t> &&
      std::is_invocable_v<ProgressFnT, IntT, IntT, IntT, uint64_t>)
std::unordered_set<IntT> parallel_blocks_multi(
    FnT&& fn,
    IntT start_value,
    IntT end_value,
    IntT block_size,
    size_t num_threads = 0,
    ProgressFnT&& progress_fn = parallel_default_progress_fn<IntT>,
    bool use_progress_fn = true) {

  if (num_threads == 0) {
    num_threads = std::thread::hardware_concurrency();
  }

  std::vector<RetT> thread_rets(num_threads);
  parallel_blocks<IntT>([&](IntT z, size_t thread_num) {
    if (fn(z, thread_num)) {
      thread_rets[thread_num].emplace(z);
    }
    return false;
  },
      start_value, end_value, block_size, num_threads, progress_fn, use_progress_fn);

  RetT ret = std::move(thread_rets[0]);
  for (size_t z = 1; z < thread_rets.size(); z++) {
    auto& thread_ret = thread_rets[z];
    ret.insert(std::make_move_iterator(thread_ret.begin()), std::make_move_iterator(thread_ret.end()));
  }

  return ret;
}

template <typename IntT = uint64_t, typename RetT = std::unordered_set<IntT>, typename FnT>
  requires(std::is_invocable_r_v<bool, FnT, IntT, size_t>)
std::unordered_set<IntT> parallel_blocks_multi(
    FnT&& fn, IntT start_value, IntT end_value, IntT block_size, size_t num_threads, std::nullptr_t) {
  return parallel_blocks_multi<IntT, RetT, FnT>(
      std::forward<FnT>(fn), start_value, end_value, block_size, num_threads, parallel_noop_progress_fn<IntT>, false);
}

// Like parallel, but iterates over a range. The range can be, for example, a std::vector or std::array.
template <std::ranges::sized_range RangeT, typename FnT>
  requires(
      std::ranges::random_access_range<RangeT> &&
      std::ranges::sized_range<RangeT> &&
      std::is_invocable_r_v<bool, FnT&, std::ranges::range_reference_t<RangeT>, size_t>)
size_t parallel_range(RangeT&& range, FnT&& fn, size_t num_threads = 0) {
  return parallel<size_t>(
      [&range, &fn](size_t z, size_t thread_num) -> bool {
        return fn(range[z], thread_num);
      },
      0,
      std::ranges::size(range),
      num_threads,
      parallel_noop_progress_fn<size_t>);
}

// Enables iteration over an enum using a range-based for loop. The enum must contain members named MIN_VALUE and
// MAX_VALUE (the latter of which will not be iterated over), but these names can be overridden via the template
// arguments.
template <typename E, E StartValue = E::MIN_VALUE, E EndValue = E::MAX_VALUE>
struct EnumRange {
  struct iterator {
    using IntT = std::underlying_type_t<E>;
    E current_value;

    E operator*() const {
      return this->current_value;
    }
    void operator++() {
      this->current_value = static_cast<E>(static_cast<IntT>(this->current_value) + 1);
    }
    bool operator==(const iterator& other) const {
      return this->current_value == other.current_value;
    }
    bool operator!=(const iterator& other) const {
      return this->current_value != other.current_value;
    }
  };

  iterator begin() const {
    return {StartValue};
  }
  iterator end() const {
    return {EndValue};
  }
};

} // namespace phosg
