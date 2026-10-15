#pragma once

#include <stdint.h>
#include <sys/types.h>

#include <stdexcept>
#include <unordered_map>

namespace phosg {

template <typename KeyT, typename ValueT>
class LRUMap {
protected:
  struct Item {
    mutable Item* prev = nullptr;
    mutable Item* next = nullptr;
    const KeyT* key = nullptr;
    ValueT value;
    size_t size;

    Item(const ValueT& value, size_t size) : value(value), size(size) {}
    Item(ValueT&& value, size_t size) : value(std::move(value)), size(size) {}
  };

  mutable Item* head = nullptr;
  mutable Item* tail = nullptr;
  std::unordered_map<KeyT, Item> items;
  size_t total_size = 0;

  void link_item(Item* i) const {
    i->next = this->head;
    if (this->head) {
      this->head->prev = i;
    }
    this->head = i;
    if (!this->tail) {
      this->tail = i;
    }
  }

  void unlink_item(Item* i) const {
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

  void touch_item(Item* item) const {
    if (this->head != item) {
      this->unlink_item(item);
      this->link_item(item);
    }
  }

  void change_item_size(Item& item, size_t new_size) {
    this->total_size += new_size - item.size;
    item.size = new_size;
  }

public:
  LRUMap() = default;
  LRUMap(const LRUMap& other) = delete;
  LRUMap(LRUMap&& other)
      : head(other.head), tail(other.tail), items(std::move(other.items)), total_size(other.total_size) {
    other.head = nullptr;
    other.tail = nullptr;
    other.total_size = 0;
  }
  LRUMap& operator=(const LRUMap& other) = delete;
  LRUMap& operator=(LRUMap&& other) {
    this->head = other.head;
    this->tail = other.tail;
    this->items = std::move(other.items);
    this->total_size = other.total_size;
    other.head = nullptr;
    other.tail = nullptr;
    other.total_size = 0;
    return *this;
  }
  virtual ~LRUMap() = default;

  ValueT& at(const KeyT& k) {
    Item& item = this->items.at(k);
    this->touch_item(&item);
    return item.value;
  }

  const ValueT& at(const KeyT& k) const {
    const Item& item = this->items.at(k);
    this->touch_item(const_cast<Item*>(&item));
    return item.value;
  }

  size_t item_size(const KeyT& k) const {
    return this->items.at(k).size;
  }

  bool insert(const KeyT& k, const ValueT& v, size_t size = 1) {
    auto [item_it, new_item_created] = this->items.emplace(k, size);
    auto& i = item_it->second;
    i.value = v;
    if (new_item_created) {
      i.key = &item_it->first;
      i.size = size;
      this->total_size += size;
      this->link_item(&i);
    } else {
      this->change_item_size(i, size);
      this->touch_item(&i);
    }
    return new_item_created;
  }

  bool insert(KeyT&& k, ValueT&& v, size_t size = 1) {
    auto [item_it, new_item_created] = this->items.try_emplace(std::move(k), std::move(v), size);
    auto& i = item_it->second;
    if (new_item_created) {
      i.key = &item_it->first;
      i.size = size;
      this->total_size += size;
      this->link_item(&i);
    } else {
      i.value = std::move(v);
      this->change_item_size(i, size);
      this->touch_item(&i);
    }
    return new_item_created;
  }

  bool emplace(KeyT&& k, ValueT&& v, size_t size = 1) {
    auto emplace_ret = this->items.try_emplace(std::move(k), std::move(v), size);

    if (emplace_ret.second) {
      // Item was inserted; set the key pointer, link the item and update the total size appropriately
      auto& i = emplace_ret.first->second;
      i.key = &emplace_ret.first->first;
      i.size = size;
      this->total_size += size;
      this->link_item(&i);
      return true;
    } else {
      return false;
    }
  }

  bool erase(const KeyT& k) {
    auto item_it = this->items.find(k);
    if (item_it == this->items.end()) {
      return false;
    }

    Item& item = item_it->second;
    this->unlink_item(&item);
    this->total_size -= item.size;
    this->items.erase(item_it);
    return true;
  }

  void clear() {
    this->head = nullptr;
    this->tail = nullptr;
    this->items.clear();
    this->total_size = 0;
  }

  bool change_size(const KeyT& k, size_t new_size, bool touch = true) {
    try {
      Item& i = this->items.at(k);
      if (touch) {
        this->touch_item(&i);
      }
      this->change_item_size(this->items.at(k), new_size);
      return true;
    } catch (const std::out_of_range& e) {
      return false;
    }
  }

  bool touch(const KeyT& k, ssize_t new_size = -1) {
    try {
      Item& i = this->items.at(k);
      this->touch_item(&i);
      if (new_size >= 0) {
        this->change_item_size(i, new_size);
      }
      return true;
    } catch (const std::out_of_range& e) {
      return false;
    }
  }

  size_t size() const {
    return this->total_size;
  }

  size_t count() const {
    return this->items.size();
  }

  bool empty() const {
    return this->items.empty();
  }

  struct EvictedObject {
    KeyT key;
    ValueT value;
    size_t size;
  };

  EvictedObject evict_object() {
    Item* i = this->tail;
    if (!i) {
      throw std::out_of_range("nothing to evict");
    }

    EvictedObject ret;
    ret.key = std::move(*i->key);
    ret.value = std::move(i->value);
    ret.size = i->size;

    this->unlink_item(i);
    this->items.erase(ret.key);
    this->total_size -= ret.size;
    return ret;
  }

  void swap(LRUMap<KeyT, ValueT>& other) {
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
