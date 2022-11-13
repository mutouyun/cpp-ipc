/**
 * @file libpmr/memory_resource.h
 * @author mutouyun (orz@orzz.org)
 * @brief Implement memory allocation strategies that can be used by pmr::allocator.
 * @date 2022-11-13
 */
#pragma once

#include <cstddef>  // std::size_t

#include "libimp/export.h"
#include "libpmr/def.h"

LIBPMR_NAMESPACE_BEG_

/**
 * @brief A memory resource that uses the 
 * global operator new and operator delete to allocate memory.
 * 
 * @see https://en.cppreference.com/w/cpp/memory/new_delete_resource
 */
class LIBIMP_EXPORT new_delete_resource {
public:
  /// @brief Allocates storage with a size of at least bytes bytes, aligned to the specified alignment.
  /// @remark Returns nullptr if storage of the requested size and alignment cannot be obtained.
  /// @see https://en.cppreference.com/w/cpp/memory/memory_resource/do_allocate
  void *do_allocate(std::size_t bytes, std::size_t alignment) noexcept;

  /// @brief Deallocates the storage pointed to by p.
  /// @see https://en.cppreference.com/w/cpp/memory/memory_resource/deallocate
  void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) noexcept;
};

LIBPMR_NAMESPACE_END_
