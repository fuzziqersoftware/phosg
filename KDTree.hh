#pragma once

#include <stdint.h>

#include <deque>
#include <memory>
#include <vector>
#include <sys/types.h>



template <typename CoordType, size_t dimensions, typename ValueType>
class KDTree {
public:
  struct Point {
    CoordType coords[dimensions];

    Point() = default;
    bool operator==(const Point& other);
    bool operator!=(const Point& other);
  };

private:
  struct Node {
    Point pt;
    size_t dim;

    Node* before;
    Node* after; // technically after_or_equal
    Node* parent;

    ValueType value;

    Node(Node* parent, const Point& pt, size_t dim, const ValueType& v);

    template<typename... Args>
    Node(const Point& pt, Args&&... args);
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
  class Iterator : public std::iterator<
      std::input_iterator_tag, std::pair<Point, ValueType>, ssize_t,
      const std::pair<Point, ValueType>*, std::pair<Point, ValueType>&> {

  public:
    explicit Iterator(Node* n);
    Iterator& operator++();
    Iterator operator++(int);
    bool operator==(const Iterator& other) const;
    bool operator!=(const Iterator& other) const;
    const std::pair<Point, ValueType>& operator*() const;
    const std::pair<Point, ValueType>* operator->() const;

  private:
    std::deque<Node*> pending;
    std::pair<Point, ValueType> current;

    friend class KDTree;
  };

  // TODO: figure out if this can be generalized with some silly template
  // shenanigans. for now we just implement some common cases and assert at
  // runtime that the right one is called. (for some reason C-style varagrs
  // doesn't work, and it's not safe anyway if the caller passes the wrong
  // argument count)
  static Point make_point(CoordType x);
  static Point make_point(CoordType x, CoordType y);
  static Point make_point(CoordType x, CoordType y, CoordType z);
  static Point make_point(CoordType x, CoordType y, CoordType z, CoordType w);

  KDTree();
  ~KDTree();

  // TODO: be unlazy
  KDTree(const KDTree&) = delete;
  KDTree(KDTree&&) = delete;
  KDTree& operator=(const KDTree&) = delete;
  KDTree& operator=(KDTree&&) = delete;

  Iterator insert(const Point& pt, const ValueType& v);

  template <typename... Args>
  Iterator emplace(const Point& pt, Args&&... args);

  bool erase(const Point& pt, const ValueType& v);
  void erase_advance(Iterator& it);

  const ValueType& at(const Point& pt) const;
  bool exists(const Point& pt) const;
  std::vector<std::pair<Point, ValueType>> within(const Point& low, const Point& high) const;
  bool exists(const Point& low, const Point& high) const;

  size_t size() const;
  size_t depth() const;

  Iterator begin() const;
  Iterator end() const;
};

#include "KDTree-inl.hh"
