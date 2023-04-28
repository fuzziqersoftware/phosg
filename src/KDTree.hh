#pragma once

#include <stdint.h>
#include <sys/types.h>

#include <deque>
#include <memory>
#include <vector>

template <typename CoordType, typename ValueType>
class KDTree {
private:
  struct Node {
    CoordType pt;
    size_t dim;

    Node* before;
    Node* after_or_equal;
    Node* parent;

    ValueType value;

    Node(Node* parent, const CoordType& pt, size_t dim, const ValueType& v);

    template <typename... Args>
    Node(const CoordType& pt, Args&&... args);
  };

  Node* root;
  size_t node_count;

  // TODO: make this not recursive
  static size_t count_subtree(const Node* n);
  static size_t depth_recursive(Node* n, size_t depth);
  void collect_into(Node* n, std::vector<Node*>& ret);

  void link_node(Node* new_node);

  bool delete_node(Node* n);

  Node* find_subtree_min_max(Node* n, size_t target_dim, bool find_max);

public:
  class Iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::pair<CoordType, ValueType>;
    using difference_type = ssize_t;
    using pointer = const value_type*;
    using reference = value_type&;

    explicit Iterator(Node* n);
    Iterator& operator++();
    Iterator operator++(int);
    bool operator==(const Iterator& other) const;
    bool operator!=(const Iterator& other) const;
    const std::pair<CoordType, ValueType>& operator*() const;
    const std::pair<CoordType, ValueType>* operator->() const;

  private:
    std::deque<Node*> pending;
    std::pair<CoordType, ValueType> current;

    friend class KDTree;
  };

  KDTree();
  ~KDTree();

  // TODO: be unlazy
  KDTree(const KDTree&) = delete;
  KDTree(KDTree&&) = delete;
  KDTree& operator=(const KDTree&) = delete;
  KDTree& operator=(KDTree&&) = delete;

  Iterator insert(const CoordType& pt, const ValueType& v);

  template <typename... Args>
  Iterator emplace(const CoordType& pt, Args&&... args);

  bool erase(const CoordType& pt, const ValueType& v);
  void erase_advance(Iterator& it);

  const ValueType& at(const CoordType& pt) const;
  bool exists(const CoordType& pt) const;
  std::vector<std::pair<CoordType, ValueType>> within(const CoordType& low,
      const CoordType& high) const;
  bool exists(const CoordType& low, const CoordType& high) const;

  size_t size() const;
  size_t depth() const;

  Iterator begin() const;
  Iterator end() const;
};

#include "KDTree-inl.hh"
