/**
 * \file libipc/memory_resource.h
 * \author mutouyun (orz@orzz.org)
 * \brief Implement memory allocation strategies that can be used by pmr::allocator.
 */
#pragma once

#include <type_traits>
#include <cstddef>  // std::size_t, std::max_align_t

#include "libipc/imp/export.h"
#include "libipc/def.h"

namespace ipc {
namespace mem {

/// \brief Helper trait for memory resource.

template <typename T, typename = void>
struct has_allocate : std::false_type {};

template <typename T>
struct has_allocate<T, 
  typename std::enable_if<std::is_convertible<
  decltype(std::declval<T &>().allocate(std::declval<std::size_t>(), 
                                        std::declval<std::size_t>())), void *
  >::value>::type> : std::true_type {};

template <typename T, typename = void>
struct has_deallocate : std::false_type {};

template <typename T>
struct has_deallocate<T, 
  decltype(std::declval<T &>().deallocate(std::declval<void *>(), 
                                          std::declval<std::size_t>(), 
                                          std::declval<std::size_t>()))
  > : std::true_type {};

template <typename T>
using verify_memory_resource = 
  std::enable_if_t<has_allocate  <T>::value && 
                   has_deallocate<T>::value, bool>;

/**
 * \class LIBIPC_EXPORT new_delete_resource
 * \brief A memory resource that uses the 
 *        standard memory allocation and deallocation interface to allocate memory.
 * \see https://en.cppreference.com/w/cpp/memory/new_delete_resource
 */
class LIBIPC_EXPORT new_delete_resource {
public:
  /// \brief Returns a pointer to a `new_delete_resource`.
  static new_delete_resource *get() noexcept;

  /// \brief Allocates storage with a size of at least bytes bytes, aligned to the specified alignment.
  /// \remark Returns nullptr if storage of the requested size and alignment cannot be obtained.
  /// \see https://en.cppreference.com/w/cpp/memory/memory_resource/do_allocate
  void *allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) noexcept;

  /// \brief Deallocates the storage pointed to by p.
  /// \see https://en.cppreference.com/w/cpp/memory/memory_resource/deallocate
  void deallocate(void *p, std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) noexcept;
};

} // namespace mem
} // namespace ipc
