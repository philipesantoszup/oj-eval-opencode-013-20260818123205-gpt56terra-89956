/**
 * A self-balancing associative container.
 */
#ifndef SJTU_MAP_HPP
#define SJTU_MAP_HPP

#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {

template<class Key, class T, class Compare = std::less<Key> >
class map {
 public:
  typedef pair<const Key, T> value_type;

 private:
  struct node {
    value_type value;
    node *left, *right, *parent;
    int height;
    node(const value_type &v, node *p) : value(v), left(nullptr), right(nullptr), parent(p), height(1) {}
  };

  node *root;
  size_t node_count;
  Compare compare;

  static int height(node *p) { return p == nullptr ? 0 : p->height; }
  static int maximum(int a, int b) { return a > b ? a : b; }
  static node *minimum(node *p) {
    while (p != nullptr && p->left != nullptr) p = p->left;
    return p;
  }
  static node *maximum(node *p) {
    while (p != nullptr && p->right != nullptr) p = p->right;
    return p;
  }
  static node *next(node *p) {
    if (p->right != nullptr) return minimum(p->right);
    node *q = p->parent;
    while (q != nullptr && p == q->right) { p = q; q = q->parent; }
    return q;
  }
  static node *previous(node *p) {
    if (p->left != nullptr) return maximum(p->left);
    node *q = p->parent;
    while (q != nullptr && p == q->left) { p = q; q = q->parent; }
    return q;
  }
  static void update(node *p) { p->height = maximum(height(p->left), height(p->right)) + 1; }

  void replace_parent_child(node *old_child, node *new_child) {
    node *p = old_child->parent;
    if (p == nullptr) root = new_child;
    else if (p->left == old_child) p->left = new_child;
    else p->right = new_child;
    if (new_child != nullptr) new_child->parent = p;
  }
  node *rotate_left(node *p) {
    node *q = p->right;
    p->right = q->left;
    if (q->left != nullptr) q->left->parent = p;
    replace_parent_child(p, q);
    q->left = p;
    p->parent = q;
    update(p); update(q);
    return q;
  }
  node *rotate_right(node *p) {
    node *q = p->left;
    p->left = q->right;
    if (q->right != nullptr) q->right->parent = p;
    replace_parent_child(p, q);
    q->right = p;
    p->parent = q;
    update(p); update(q);
    return q;
  }
  void rebalance(node *p) {
    while (p != nullptr) {
      update(p);
      int balance = height(p->left) - height(p->right);
      if (balance > 1) {
        if (height(p->left->left) < height(p->left->right)) rotate_left(p->left);
        p = rotate_right(p);
      } else if (balance < -1) {
        if (height(p->right->right) < height(p->right->left)) rotate_right(p->right);
        p = rotate_left(p);
      }
      p = p->parent;
    }
  }
  node *locate(const Key &key) const {
    node *p = root;
    while (p != nullptr) {
      if (compare(key, p->value.first)) p = p->left;
      else if (compare(p->value.first, key)) p = p->right;
      else return p;
    }
    return nullptr;
  }
  static void destroy(node *p) {
    if (p == nullptr) return;
    destroy(p->left); destroy(p->right); delete p;
  }
  static node *copy_tree(node *source, node *parent) {
    if (source == nullptr) return nullptr;
    node *result = new node(source->value, parent);
    try {
      result->left = copy_tree(source->left, result);
      result->right = copy_tree(source->right, result);
      result->height = source->height;
    } catch (...) {
      destroy(result);
      throw;
    }
    return result;
  }

 public:
  class const_iterator;
  class iterator {
    friend class map;
    friend class const_iterator;
    node *current;
    map *owner;
    iterator(node *p, map *m) : current(p), owner(m) {}
   public:
    iterator() : current(nullptr), owner(nullptr) {}
    iterator(const iterator &other) : current(other.current), owner(other.owner) {}
    iterator operator++(int) { iterator result(*this); ++*this; return result; }
    iterator &operator++() {
      if (owner == nullptr || current == nullptr) throw invalid_iterator();
      current = next(current); return *this;
    }
    iterator operator--(int) { iterator result(*this); --*this; return result; }
    iterator &operator--() {
      if (owner == nullptr) throw invalid_iterator();
      if (current == nullptr) current = maximum(owner->root);
      else current = previous(current);
      if (current == nullptr) throw invalid_iterator();
      return *this;
    }
    value_type &operator*() const {
      if (owner == nullptr || current == nullptr) throw invalid_iterator();
      return current->value;
    }
    value_type *operator->() const noexcept { return current == nullptr ? nullptr : &current->value; }
    bool operator==(const iterator &rhs) const { return current == rhs.current && owner == rhs.owner; }
    bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
    bool operator==(const const_iterator &rhs) const;
    bool operator!=(const const_iterator &rhs) const;
  };

  class const_iterator {
    friend class map;
    friend class iterator;
    node *current;
    const map *owner;
    const_iterator(node *p, const map *m) : current(p), owner(m) {}
   public:
    const_iterator() : current(nullptr), owner(nullptr) {}
    const_iterator(const const_iterator &other) : current(other.current), owner(other.owner) {}
    const_iterator(const iterator &other) : current(other.current), owner(other.owner) {}
    const_iterator operator++(int) { const_iterator result(*this); ++*this; return result; }
    const_iterator &operator++() {
      if (owner == nullptr || current == nullptr) throw invalid_iterator();
      current = next(current); return *this;
    }
    const_iterator operator--(int) { const_iterator result(*this); --*this; return result; }
    const_iterator &operator--() {
      if (owner == nullptr) throw invalid_iterator();
      if (current == nullptr) current = maximum(owner->root);
      else current = previous(current);
      if (current == nullptr) throw invalid_iterator();
      return *this;
    }
    const value_type &operator*() const {
      if (owner == nullptr || current == nullptr) throw invalid_iterator();
      return current->value;
    }
    const value_type *operator->() const noexcept { return current == nullptr ? nullptr : &current->value; }
    bool operator==(const const_iterator &rhs) const { return current == rhs.current && owner == rhs.owner; }
    bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }
    bool operator==(const iterator &rhs) const { return current == rhs.current && owner == rhs.owner; }
    bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
  };

  map() : root(nullptr), node_count(0), compare() {}
  map(const map &other) : root(nullptr), node_count(other.node_count), compare(other.compare) {
    root = copy_tree(other.root, nullptr);
  }
  map &operator=(const map &other) {
    if (this == &other) return *this;
    node *new_root = copy_tree(other.root, nullptr);
    destroy(root);
    root = new_root; node_count = other.node_count; compare = other.compare;
    return *this;
  }
  ~map() { destroy(root); }

  T &at(const Key &key) {
    node *p = locate(key); if (p == nullptr) throw index_out_of_bound(); return p->value.second;
  }
  const T &at(const Key &key) const {
    node *p = locate(key); if (p == nullptr) throw index_out_of_bound(); return p->value.second;
  }
  T &operator[](const Key &key) {
    node *p = locate(key);
    if (p == nullptr) p = insert(value_type(key, T())).first.current;
    return p->value.second;
  }
  const T &operator[](const Key &key) const { return at(key); }
  iterator begin() { return iterator(minimum(root), this); }
  const_iterator cbegin() const { return const_iterator(minimum(root), this); }
  iterator end() { return iterator(nullptr, this); }
  const_iterator cend() const { return const_iterator(nullptr, this); }
  bool empty() const { return node_count == 0; }
  size_t size() const { return node_count; }
  void clear() { destroy(root); root = nullptr; node_count = 0; }
  pair<iterator, bool> insert(const value_type &value) {
    if (root == nullptr) { root = new node(value, nullptr); ++node_count; return pair<iterator, bool>(iterator(root, this), true); }
    node *p = root, *parent = nullptr;
    while (p != nullptr) {
      parent = p;
      if (compare(value.first, p->value.first)) p = p->left;
      else if (compare(p->value.first, value.first)) p = p->right;
      else return pair<iterator, bool>(iterator(p, this), false);
    }
    node *added = new node(value, parent);
    if (compare(value.first, parent->value.first)) parent->left = added;
    else parent->right = added;
    ++node_count; rebalance(parent);
    return pair<iterator, bool>(iterator(added, this), true);
  }
  void erase(iterator pos) {
    if (pos.owner != this || pos.current == nullptr) throw invalid_iterator();
    node *p = pos.current;
    if (p->left != nullptr && p->right != nullptr) {
      node *successor = minimum(p->right);
      node *successor_parent = successor->parent;
      node *successor_right = successor->right;
      replace_parent_child(successor, successor_right);
      successor->left = p->left; successor->left->parent = successor;
      successor->right = p->right;
      if (successor->right != nullptr) successor->right->parent = successor;
      replace_parent_child(p, successor);
      successor->height = p->height;
      delete p; --node_count;
      rebalance(successor_parent == p ? successor : successor_parent);
    } else {
      node *parent = p->parent;
      replace_parent_child(p, p->left != nullptr ? p->left : p->right);
      delete p; --node_count; rebalance(parent);
    }
  }
  size_t count(const Key &key) const { return locate(key) == nullptr ? 0 : 1; }
  iterator find(const Key &key) { return iterator(locate(key), this); }
  const_iterator find(const Key &key) const { return const_iterator(locate(key), this); }
};

template<class Key, class T, class Compare>
bool map<Key, T, Compare>::iterator::operator==(const const_iterator &rhs) const {
  return current == rhs.current && owner == rhs.owner;
}
template<class Key, class T, class Compare>
bool map<Key, T, Compare>::iterator::operator!=(const const_iterator &rhs) const { return !(*this == rhs); }

}

#endif
