#pragma once

#include <stdint.h>
#include <sys/types.h>

#include <unordered_map>

template <typename K>
class LRUSet {
protected:
  struct Item {
    Item* prev;
    Item* next;
    const K* key;
    size_t size;

    Item(size_t size);
  };

  Item* head;
  Item* tail;
  std::unordered_map<K, Item> items;
  size_t total_size;

  bool after_emplace(
      const std::pair<typename std::unordered_map<K, Item>::iterator, bool>& emplace_ret, size_t size);

  void unlink_item(Item* i);
  void link_item(Item* i);

public:
  LRUSet();
  virtual ~LRUSet();

  bool insert(const K& k, size_t size = 0);
  bool emplace(K&& k, size_t size = 0);

  bool erase(const K& k);
  void clear();

  bool change_size(const K& k, size_t new_size);
  bool touch(const K& k, ssize_t new_size = -1);

  size_t size() const;
  size_t count() const;

  std::pair<K, size_t> evict_object();
  std::pair<K, size_t> peek();

  void swap(LRUSet<K>& other);
};

#include "LRUSet-inl.hh"
