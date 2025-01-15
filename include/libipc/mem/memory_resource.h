/**
 * \file libipc/memory_resource.h
 * \author mutouyun (orz@orzz.org)
 * \brief Implement memory allocation strategies that can be used by ipc::mem::bytes_allocator.
 */
#pragma once

#include <type_traits>
#include <cstddef>  // std::size_t, std::max_align_t

#include "libipc/imp/export.h"
#include "libipc/imp/span.h"
#include "libipc/imp/byte.h"
#include "libipc/mem/polymorphic_allocator.h"

namespace ipc {
namespace mem {

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

/**
 * \class LIBIPC_EXPORT monotonic_buffer_resource
 * \brief A special-purpose memory resource class 
 *        that releases the allocated memory only when the resource is destroyed.
 * \see https://en.cppreference.com/w/cpp/memory/monotonic_buffer_resource
 */
class LIBIPC_EXPORT monotonic_buffer_resource {

  bytes_allocator upstream_;

  struct node {
    node *next;
    std::size_t size;
  } *free_list_;

  ipc::byte * head_;
  ipc::byte * tail_;
  std::size_t next_size_;

  ipc::byte * const initial_buffer_;
  std::size_t const initial_size_;

public:
  monotonic_buffer_resource() noexcept;
  explicit monotonic_buffer_resource(bytes_allocator upstream) noexcept;
  explicit monotonic_buffer_resource(std::size_t initial_size) noexcept;
  monotonic_buffer_resource(std::size_t initial_size, bytes_allocator upstream) noexcept;
  monotonic_buffer_resource(ipc::span<ipc::byte> buffer) noexcept;
  monotonic_buffer_resource(ipc::span<ipc::byte> buffer, bytes_allocator upstream) noexcept;

  ~monotonic_buffer_resource() noexcept;

  monotonic_buffer_resource(monotonic_buffer_resource const &) = delete;
  monotonic_buffer_resource &operator=(monotonic_buffer_resource const &) = delete;

  bytes_allocator upstream_resource() const noexcept;
  void release() noexcept;

  void *allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) noexcept;
  void deallocate(void *p, std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) noexcept;
};

} // namespace mem
} // namespace ipc
