#pragma once

#include <stdint.h>

#include <deque>
#include <memory>
#include <vector>



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

    Node(const Point& pt, size_t dim, const ValueType& v, Node* parent = NULL);
  };

  Node* root;
  size_t node_count;

  // TODO: make this not recursive
  static size_t count_subtree(const Node* n);
  static size_t depth_recursive(Node* n, size_t depth);
  void collect_into(Node* n, std::vector<Node*>& ret);

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

  void insert(const Point& pt, const ValueType& v);
  bool erase(const Point& pt, const ValueType& v);
  void erase_advance(Iterator& it);

  const ValueType& at(const Point& pt) const;
  bool exists(const Point& pt) const;
  std::vector<std::pair<Point, ValueType>> within(const Point& low, const Point& high) const;

  size_t size() const;
  size_t depth() const;

  Iterator begin() const;
  Iterator end() const;

  // TODO remove this
  void print_structure_int64_2(FILE* stream) const;
  void print_structure_int64_2_recursive(FILE* stream, const Node* n, size_t depth, const char* label) const;
};

#include "KDTree-inl.hh"
