/**
 * \file libconcur/intrusive_stack.h
 * \author mutouyun (orz@orzz.org)
 * \brief Define concurrent intrusive stack.
 * \date 2023-11-18
 */
#pragma once

#include <atomic>

#include "libconcur/def.h"

LIBCONCUR_NAMESPACE_BEG_

/// \brief Intrusive stack node.
/// \tparam T The type of the value.
template <typename T>
struct intrusive_node {
  T value;
  std::atomic<intrusive_node *> next;
};

/// \brief Intrusive stack.
/// \tparam T The type of the value.
/// \tparam Node The type of the node.
template <typename T, typename Node = intrusive_node<T>>
class intrusive_stack {
public:
  using node = Node;

private:
  std::atomic<node *> top_{nullptr};

public:
  intrusive_stack(intrusive_stack const &) = delete;
  intrusive_stack(intrusive_stack &&) = delete;
  intrusive_stack &operator=(intrusive_stack const &) = delete;
  intrusive_stack &operator=(intrusive_stack &&) = delete;

  constexpr intrusive_stack() noexcept = default;

  bool empty() const noexcept {
    return top_.load(std::memory_order_acquire) == nullptr;
  }

  void push(node *n) noexcept {
    node *old_top = top_.load(std::memory_order_acquire);
    do {
      n->next.store(old_top, std::memory_order_relaxed);
    } while (!top_.compare_exchange_weak(old_top, n, std::memory_order_release
                                                   , std::memory_order_acquire));
  }

  node *pop() noexcept {
    node *old_top = top_.load(std::memory_order_acquire);
    do {
      if (old_top == nullptr) {
        return nullptr;
      }
    } while (!top_.compare_exchange_weak(old_top, old_top->next.load(std::memory_order_relaxed)
                                                , std::memory_order_release
                                                , std::memory_order_acquire));
    return old_top;
  }
};

LIBCONCUR_NAMESPACE_END_
