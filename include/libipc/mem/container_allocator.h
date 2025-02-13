/**
 * \file libipc/container_allocator.h
 * \author mutouyun (orz@orzz.org)
 * \brief An allocator that can be used by all standard library containers.
 */
#pragma once

#include <cstddef>
#include <utility>

#include "libipc/imp/uninitialized.h"
#include "libipc/mem/new.h"

namespace ipc {
namespace mem {

/**
 * \brief An allocator that can be used by all standard library containers.
 * 
 * \see https://en.cppreference.com/w/cpp/memory/allocator
 *      https://en.cppreference.com/w/cpp/memory/polymorphic_allocator
 */
template <typename T>
class container_allocator {

  template <typename U>
  friend class container_allocator;

public:
  // type definitions
  typedef T                 value_type;
  typedef value_type *      pointer;
  typedef const value_type *const_pointer;
  typedef value_type &      reference;
  typedef const value_type &const_reference;
  typedef std::size_t       size_type;
  typedef std::ptrdiff_t    difference_type;

  // the other type of std_allocator
  template <typename U>
  struct rebind { 
    using other = container_allocator<U>;
  };

  container_allocator() noexcept {}

  // construct by copying (do nothing)
  container_allocator           (container_allocator<T> const &) noexcept {}
  container_allocator& operator=(container_allocator<T> const &) noexcept { return *this; }

  // construct from a related allocator (do nothing)
  template <typename U> container_allocator           (container_allocator<U> const &) noexcept {}
  template <typename U> container_allocator &operator=(container_allocator<U> const &) noexcept { return *this; }

  container_allocator           (container_allocator &&) noexcept = default;
  container_allocator& operator=(container_allocator &&) noexcept = default;

  constexpr size_type max_size() const noexcept {
    return 1;
  }

  pointer allocate(size_type count) noexcept {
    if (count == 0) return nullptr;
    if (count > this->max_size()) return nullptr;
    return mem::$new<value_type>();
  }

  void deallocate(pointer p, size_type count) noexcept {
    if (count == 0) return;
    if (count > this->max_size()) return;
    mem::$delete(p);
  }

  template <typename... P>
  static void construct(pointer p, P && ... params) {
    std::ignore = ipc::construct<T>(p, std::forward<P>(params)...);
  }

  static void destroy(pointer p) {
    std::ignore = ipc::destroy(p);
  }
};

template <typename T, typename U>
constexpr bool operator==(container_allocator<T> const &, container_allocator<U> const &) noexcept {
  return true;
}

template <typename T, typename U>
constexpr bool operator!=(container_allocator<T> const &, container_allocator<U> const &) noexcept {
  return false;
}

} // namespace mem
} // namespace ipc
