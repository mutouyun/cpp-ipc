/**
 * \file libpmr/monotonic_buffer_resource.h
 * \author mutouyun (orz@orzz.org)
 * \brief A special-purpose the `allocator` that releases the allocated memory only when the resource is destroyed.
 * \date 2023-09-16
 */
#pragma once

#include <cstddef>  // std::size_t, std::max_align_t

#include "libimp/export.h"
#include "libimp/span.h"
#include "libimp/byte.h"

#include "libpmr/def.h"
#include "libpmr/allocator.h"

LIBPMR_NAMESPACE_BEG_

/**
 * \class LIBIMP_EXPORT monotonic_buffer_resource
 * \brief A special-purpose memory resource class 
 *        that releases the allocated memory only when the resource is destroyed.
 * \see https://en.cppreference.com/w/cpp/memory/monotonic_buffer_resource
 */
class LIBIMP_EXPORT monotonic_buffer_resource {

  allocator upstream_;

  struct node {
    node *next;
    std::size_t size;
  } *free_list_;

  ::LIBIMP::byte *head_;
  ::LIBIMP::byte *tail_;
  std::size_t next_size_;

  ::LIBIMP::byte * const initial_buffer_;
  std::size_t const initial_size_;

public:
  monotonic_buffer_resource() noexcept;
  explicit monotonic_buffer_resource(allocator upstream) noexcept;
  explicit monotonic_buffer_resource(std::size_t initial_size) noexcept;
  monotonic_buffer_resource(std::size_t initial_size, allocator upstream) noexcept;
  monotonic_buffer_resource(::LIBIMP::span<::LIBIMP::byte> buffer) noexcept;
  monotonic_buffer_resource(::LIBIMP::span<::LIBIMP::byte> buffer, allocator upstream) noexcept;

  ~monotonic_buffer_resource() noexcept;

  monotonic_buffer_resource(monotonic_buffer_resource const &) = delete;
  monotonic_buffer_resource &operator=(monotonic_buffer_resource const &) = delete;

  allocator upstream_resource() const noexcept;
  void release() noexcept;

  void *allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) noexcept;
  void deallocate(void *p, std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) noexcept;
};

LIBPMR_NAMESPACE_END_
