#pragma once

#include <stdint.h>
#include <sys/types.h>

#include <array>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace phosg {

// This class implements a bitset with an index on top of it, which can be used to find the next or previous set bit
// (after or before a given position) in time logarithmic in the total bitset size. For any practical bitset size, this
// amounts to only a few memory accesses and arithmetic/bitwise operations. It is up to the caller to ensure they don't
// pass any position greater than or equal to the bit count to any function call.

template <size_t Count>
struct SearchableBitSet;

template <size_t Count> requires(Count > 0 && Count <= 0x40)
struct SearchableBitSet<Count> {
  uint64_t data = 0;

  SearchableBitSet(uint64_t data = 0) : data(data) {}
  SearchableBitSet(const SearchableBitSet<Count>&) = default;
  SearchableBitSet(SearchableBitSet<Count>&&) = default;
  SearchableBitSet& operator=(const SearchableBitSet<Count>&) = default;
  SearchableBitSet& operator=(SearchableBitSet<Count>&&) = default;

  bool operator==(const SearchableBitSet<Count>& other) const {
    return (this->data == other.data);
  }
  bool operator!=(const SearchableBitSet<Count>& other) const {
    return !this->operator==(other);
  }

  static constexpr uint64_t bit_for_pos(size_t pos) {
    return (1ULL << (63 - (pos & 63)));
  }

  static consteval size_t num_levels() {
    return 1;
  }

  void clear_all() {
    this->data = 0;
  }
  void set_all() {
    this->data = 0xFFFFFFFFFFFFFFFFULL << (0x40 - Count);
  }
  constexpr bool any_set() const {
    return (this->data != 0);
  }
  constexpr size_t count() const {
    return Count;
  }

  constexpr bool check(size_t pos) const {
    return this->data & bit_for_pos(pos);
  }
  void set(size_t pos) {
    this->data |= bit_for_pos(pos);
  }
  void clear(size_t pos) {
    this->data &= ~bit_for_pos(pos);
  }

  constexpr size_t find_set_at_or_after(size_t pos) const {
    // Returns Count if no bits are set after the specified position
    size_t left_zeroes = std::countl_zero(this->data & (bit_for_pos(pos) | (bit_for_pos(pos) - 1)));
    return (left_zeroes == 64) ? Count : left_zeroes;
  }
  constexpr size_t find_set_at_or_before(size_t pos) const {
    // Returns Count if no bits are set before the specified position
    size_t right_zeroes = std::countr_zero(this->data & ~(bit_for_pos(pos) - 1));
    return (right_zeroes == 64) ? Count : (63 - right_zeroes);
  }
};
static_assert(sizeof(SearchableBitSet<64>) == 8, "SearchableBitSet base size is incorrect");

template <size_t Count> requires(Count > 0x40)
struct SearchableBitSet<Count> {
  static constexpr size_t DataCount = (Count + 63) / 64;

  SearchableBitSet<DataCount> index; // Each bit is set if the corresponding entry in this->data has any set bits
  std::array<SearchableBitSet<64>, DataCount> data = {};

  SearchableBitSet() = default;
  SearchableBitSet(const SearchableBitSet<Count>&) = default;
  SearchableBitSet(SearchableBitSet<Count>&&) = default;
  SearchableBitSet& operator=(const SearchableBitSet<Count>&) = default;
  SearchableBitSet& operator=(SearchableBitSet<Count>&&) = default;

  bool operator==(const SearchableBitSet<Count>& other) const {
    return (this->data == other.data);
  }
  bool operator!=(const SearchableBitSet<Count>& other) const {
    return !this->operator==(other);
  }

  static consteval size_t num_levels() {
    return 1 + SearchableBitSet<DataCount>::num_levels();
  }

  void clear_all() {
    this->index.clear_all();
    for (size_t z = 0; z < this->data.size(); z++) {
      this->data[z].clear_all();
    }
  }
  void set_all() {
    this->index.set_all();
    for (size_t z = 0; z < this->data.size(); z++) {
      this->data[z].set_all();
    }
  }
  constexpr bool any_set() const {
    return this->index.any_set();
  }
  constexpr size_t count() const {
    return Count;
  }

  constexpr bool check(size_t pos) const {
    return this->data[pos / 64].check(pos & 63);
  }
  void set(size_t pos) {
    size_t data_index = pos / 64;
    this->data[data_index].set(pos & 63);
    this->index.set(data_index);
  }
  void clear(size_t pos) {
    size_t data_index = pos / 64;
    this->data[data_index].clear(pos & 63);
    if (!this->data[data_index].any_set()) {
      this->index.clear(data_index);
    }
  }

  constexpr size_t find_set_at_or_after(size_t pos) const {
    size_t data_index = pos / 64;
    size_t ret = this->data[data_index].find_set_at_or_after(pos & 63);
    if (ret < 64) {
      return (data_index * 64) + ret;
    } else if (data_index == this->data.size() - 1) {
      return this->count();
    } else {
      data_index = this->index.find_set_at_or_after(data_index + 1);
      return (data_index == this->data.size()) ? Count : ((data_index * 64) + this->data[data_index].find_set_at_or_after(0));
    }
  }
  constexpr size_t find_set_at_or_before(size_t pos) const {
    size_t data_index = pos / 64;
    size_t ret = this->data[data_index].find_set_at_or_before(pos & 63);
    if (ret < 64) {
      return (data_index * 64) + ret;
    } else if (data_index == 0) {
      return this->count();
    } else {
      data_index = this->index.find_set_at_or_before(data_index - 1);
      return (data_index == this->data.size()) ? Count : ((data_index * 64) + this->data[data_index].find_set_at_or_before(63));
    }
  }
};

struct SearchableBitVector {
  std::unique_ptr<SearchableBitVector> index;
  std::vector<SearchableBitSet<64>> data;
  size_t bit_count;

  inline SearchableBitVector() : bit_count(0) {}

  inline SearchableBitVector(size_t bit_count) : bit_count(bit_count) {
    size_t data_count = std::max<size_t>((this->bit_count + 63) / 64, 1);
    this->data.resize(data_count);
    if (data_count > 1) {
      this->index = std::make_unique<SearchableBitVector>(data_count);
    }
  }
  inline SearchableBitVector(const SearchableBitVector& other)
      : index(other.index ? std::make_unique<SearchableBitVector>(*other.index) : nullptr),
        data(other.data),
        bit_count(other.bit_count) {}
  inline SearchableBitVector(SearchableBitVector&& other)
      : index(std::move(other.index)), data(std::move(other.data)), bit_count(other.bit_count) {
    other.bit_count = 0;
  }
  inline SearchableBitVector& operator=(const SearchableBitVector& other) {
    if (other.index) {
      this->index = std::make_unique<SearchableBitVector>(*other.index);
    } else {
      this->index.reset();
    }
    this->data = other.data;
    this->bit_count = other.bit_count;
    return *this;
  }
  inline SearchableBitVector& operator=(SearchableBitVector&& other) {
    this->index = std::move(other.index);
    this->data = std::move(other.data);
    this->bit_count = other.bit_count;
    other.bit_count = 0;
    return *this;
  }

  bool operator==(const SearchableBitVector& other) const {
    return (this->bit_count == other.bit_count) && (this->data == other.data);
  }
  bool operator!=(const SearchableBitVector& other) const {
    return !this->operator==(other);
  }

  inline size_t num_levels() const {
    return 1 + (this->index ? this->index->num_levels() : 0);
  }

  inline void clear_all() {
    if (this->index) {
      this->index->clear_all();
    }
    for (size_t z = 0; z < this->data.size(); z++) {
      this->data[z].clear_all();
    }
  }
  inline void set_all() {
    if (this->index) {
      this->index->set_all();
    }
    for (size_t z = 0; z < this->data.size() - 1; z++) {
      this->data[z].set_all();
    }
    size_t last_data_bits = (this->bit_count & 0x3F);
    if (last_data_bits == 0) { // Size is a multiple of 64 bits
      this->data.back().set_all();
    } else {
      this->data.back().data = 0xFFFFFFFFFFFFFFFFULL << (64 - last_data_bits);
    }
  }
  inline bool any_set() const {
    return this->index ? this->index->any_set() : this->data[0].any_set();
  }
  constexpr size_t count() const {
    return this->bit_count;
  }

  inline bool check(size_t pos) const {
    return this->data[pos / 64].check(pos & 63);
  }
  inline void set(size_t pos) {
    size_t data_index = pos / 64;
    this->data[data_index].set(pos & 63);
    if (this->index) {
      this->index->set(data_index);
    }
  }
  inline void clear(size_t pos) {
    size_t data_index = pos / 64;
    this->data[data_index].clear(pos & 63);
    if (this->index && !this->data[data_index].any_set()) {
      this->index->clear(data_index);
    }
  }
  inline size_t find_set_at_or_after(size_t pos) const {
    size_t data_index = pos / 64;
    size_t ret = this->data[data_index].find_set_at_or_after(pos & 63);
    if (ret < 64) {
      return (data_index * 64) + ret;
    } else if (!this->index || (data_index == this->data.size() - 1)) {
      return this->bit_count;
    } else {
      data_index = this->index->find_set_at_or_after(data_index + 1);
      return (data_index == this->data.size())
          ? this->bit_count
          : ((data_index * 64) + this->data[data_index].find_set_at_or_after(0));
    }
  }
  inline size_t find_set_at_or_before(size_t pos) const {
    size_t data_index = pos / 64;
    size_t ret = this->data[data_index].find_set_at_or_before(pos & 63);
    if (ret < 64) {
      return (data_index * 64) + ret;
    } else if (!this->index || (data_index == 0)) {
      return this->bit_count;
    } else {
      data_index = this->index->find_set_at_or_before(data_index - 1);
      return (data_index == this->data.size())
          ? this->bit_count
          : ((data_index * 64) + this->data[data_index].find_set_at_or_before(63));
    }
  }
};

} // namespace phosg
