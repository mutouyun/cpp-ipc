/**
 * \file libpmr/synchronized_pool_resource.h
 * \author mutouyun (orz@orzz.org)
 * \brief A thread-safe std::pmr::memory_resource for managing allocations in pools of different block sizes.
 * \date 2023-12-23
 */
#pragma once

#include <cstddef>  // std::size_t, std::max_align_t

#include "libimp/export.h"
#include "libpmr/def.h"

LIBPMR_NAMESPACE_BEG_

/**
 * \class LIBIMP_EXPORT synchronized_pool_resource
 * \brief `synchronized_pool_resource` may be accessed from multiple threads without external synchronization, 
 *        and have thread-specific pools to reduce synchronization costs.
 * \note Unlike the standard library implementation, `synchronized_pool_resource` automatically manages 
 *       the block size and reclaims all allocated memory to the central heap when the thread is destroyed, 
 *       rather than being destructed on its own.
 * \see https://en.cppreference.com/w/cpp/memory/synchronized_pool_resource
 */
class LIBIMP_EXPORT synchronized_pool_resource {
public:
  /// \brief Returns a pointer to a `synchronized_pool_resource`.
  static synchronized_pool_resource *get() noexcept;

  /// \brief Allocates storage with a size of at least bytes bytes, aligned to the specified alignment.
  /// \remark Returns nullptr if storage of the requested size and alignment cannot be obtained.
  /// \see https://en.cppreference.com/w/cpp/memory/memory_resource/do_allocate
  void *allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) noexcept;

  /// \brief Deallocates the storage pointed to by p.
  /// \see https://en.cppreference.com/w/cpp/memory/memory_resource/deallocate
  void deallocate(void *p, std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) noexcept;
};

LIBPMR_NAMESPACE_END_
