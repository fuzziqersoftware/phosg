#pragma once

#include <stdint.h>
#include <sys/types.h>

#include <stdexcept>
#include <unordered_map>

namespace phosg {

template <typename K>
class LRUSet {
protected:
  struct Item {
    Item* prev = nullptr;
    Item* next = nullptr;
    const K* key = nullptr;
    size_t size;

    Item(size_t size) : size(size) {}
  };

  Item* head = nullptr;
  Item* tail = nullptr;
  std::unordered_map<K, Item> items;
  size_t total_size = 0;

  bool after_emplace(const std::pair<typename std::unordered_map<K, Item>::iterator, bool>& emplace_ret, size_t size) {
    auto& [k, i] = *emplace_ret.first;
    if (emplace_ret.second) { // Item was inserted
      i.key = &k;
      i.size = size;
      this->total_size += size;
      this->link_item(&i);
      return true;
    } else { // Item already existed
      // The key must match already; just update the size and move the item to the front of the LRU
      ssize_t size_delta = static_cast<ssize_t>(size) - static_cast<ssize_t>(i.size);
      i.size = size;
      this->total_size += size_delta;
      this->unlink_item(&i);
      this->link_item(&i);
      return false;
    }
  }

  void unlink_item(Item* i) {
    if (this->head == i) {
      this->head = i->next;
    }
    if (this->tail == i) {
      this->tail = i->prev;
    }
    if (i->prev) {
      i->prev->next = i->next;
    }
    if (i->next) {
      i->next->prev = i->prev;
    }
    i->prev = nullptr;
    i->next = nullptr;
  }

  void link_item(Item* i) {
    i->next = this->head;
    if (this->head) {
      this->head->prev = i;
    }
    this->head = i;
    if (!this->tail) {
      this->tail = i;
    }
  }

public:
  LRUSet() = default;
  LRUSet(const LRUSet&) = delete;
  LRUSet(LRUSet&& other)
      : head(other.head), tail(other.tail), items(std::move(other.items)), total_size(other.total_size) {
    other.head = nullptr;
    other.tail = nullptr;
    other.total_size = 0;
  }
  LRUSet& operator=(const LRUSet&) = delete;
  LRUSet& operator=(LRUSet&& other) {
    this->head = other.head;
    this->tail = other.tail;
    this->items = std::move(other.items);
    this->total_size = other.total_size;
    other.head = nullptr;
    other.tail = nullptr;
    other.total_size = 0;
    return *this;
  }

  virtual ~LRUSet() = default;

  bool insert(const K& k, size_t size = 0) {
    return this->after_emplace(this->items.emplace(k, size), size);
  }

  bool emplace(K&& k, size_t size = 0) {
    return this->after_emplace(this->items.emplace(std::move(k), size), size);
  }

  bool erase(const K& k) {
    if (auto item_it = this->items.find(k); item_it != this->items.end()) {
      Item& item = item_it->second;
      this->unlink_item(&item);
      this->total_size -= item.size;
      this->items.erase(item_it);
      return true;
    } else {
      return false;
    }
  }

  void clear() {
    this->head = nullptr;
    this->tail = nullptr;
    this->items.clear();
    this->total_size = 0;
  }

  bool change_size(const K& k, size_t new_size) {
    if (auto it = this->items.find(k); it != this->items.end()) {
      Item& i = it->second;
      this->total_size += new_size - i.size;
      i.size = new_size;
      return true;
    } else {
      return false;
    }
  }

  bool touch(const K& k, ssize_t new_size = -1) {
    if (auto it = this->items.find(k); it != this->items.end()) {
      Item& i = it->second;
      if (this->head != &i) {
        this->unlink_item(&i);
        this->link_item(&i);
      }
      if (new_size >= 0) {
        this->total_size += new_size - i.size;
        i.size = new_size;
      }
      return true;
    } else {
      return false;
    }
  }

  size_t size() const {
    return this->total_size;
  }

  size_t count() const {
    return this->items.size();
  }

  std::pair<K, size_t> evict_object() {
    Item* i = this->tail;
    if (!i) {
      throw std::out_of_range("Nothing to evict");
    }

    this->unlink_item(i);
    std::pair<K, size_t> ret = std::make_pair(*i->key, i->size);

    this->items.erase(ret.first);
    this->total_size -= ret.second;
    return ret;
  }

  std::pair<K, size_t> peek() {
    Item* i = this->tail;
    if (!i) {
      throw std::out_of_range("set is empty");
    }
    return std::make_pair(*i->key, i->size);
  }

  void swap(LRUSet<K>& other) {
    Item* this_head = this->head;
    Item* this_tail = this->tail;
    size_t this_total_size = this->total_size;

    this->items.swap(other.items);
    this->head = other.head;
    this->tail = other.tail;
    this->total_size = other.total_size;

    other.head = this_head;
    other.tail = this_tail;
    other.total_size = this_total_size;
  }
};

} // namespace phosg
