/**
 * \file libpmr/new.h
 * \author mutouyun (orz@orzz.org)
 * \brief Global object creation function.
 * \date 2024-01-01
 */
#pragma once

#include <cstddef>
#include <unordered_map>
#include <algorithm>

#include "libimp/aligned.h"
#include "libimp/uninitialized.h"
#include "libimp/byte.h"
#include "libimp/detect_plat.h"
#include "libimp/export.h"

#include "libpmr/def.h"
#include "libpmr/block_pool.h"
#include "libpmr/memory_resource.h"

LIBPMR_NAMESPACE_BEG_

/// \brief Select the incremental level based on the size.
constexpr inline std::size_t regular_level(std::size_t s) noexcept {
  return (s <= 128  ) ? 0 :
         (s <= 1024 ) ? 1 :
         (s <= 8192 ) ? 2 :
         (s <= 65536) ? 3 : 4;
}

/// \brief Calculates the appropriate memory block size based on the increment level and size.
constexpr inline std::size_t regular_sizeof_impl(std::size_t l, std::size_t s) noexcept {
  return (l == 0) ? std::max<std::size_t>(::LIBIMP::round_up<std::size_t>(s, 8), regular_head_size) :
         (l == 1) ? ::LIBIMP::round_up<std::size_t>(s, 128 ) :
         (l == 2) ? ::LIBIMP::round_up<std::size_t>(s, 1024) :
         (l == 3) ? ::LIBIMP::round_up<std::size_t>(s, 8192) : (std::numeric_limits<std::size_t>::max)();
}

/// \brief Calculates the appropriate memory block size based on the size.
constexpr inline std::size_t regular_sizeof(std::size_t s) noexcept {
  return regular_sizeof_impl(regular_level(s), s);
}

/// \brief Calculates the appropriate memory block size based on the specific type.
template <typename T>
constexpr inline std::size_t regular_sizeof() noexcept {
  return regular_sizeof(regular_head_size + sizeof(T));
}

/// \brief Defines the memory block collector interface.
class LIBIMP_EXPORT block_collector {
 public:
  virtual ~block_collector() noexcept = default;
  virtual void deallocate(void *p) noexcept = 0;
};

/// \brief Gets all block pools of the thread cache.
LIBIMP_EXPORT auto get_thread_block_pool_map() noexcept
  -> std::unordered_map<std::size_t, block_collector *> &;

/// \brief Defines block pool memory resource based on block pool.
template <std::size_t BlockSize, std::size_t BlockPoolExpansion>
class block_pool_resource;

/// \brief Memory block collector of unknown size.
/// \note This memory resource is only used to temporarily collect memory blocks 
///       that cannot find a suitable block pool memory resource.
template <>
class block_pool_resource<0, 0> : public block_pool<0, 0>
                                , public block_collector {
public:
  void deallocate(void *p) noexcept override {
    block_pool<0, 0>::deallocate(p);
  }
};

/// \brief A block pool memory resource for a block of memory of a specific size.
template <std::size_t BlockSize, std::size_t BlockPoolExpansion>
class block_pool_resource : public block_pool<BlockSize, BlockPoolExpansion>
                          , public block_collector {

  using base_t = block_pool<BlockSize, BlockPoolExpansion>;

  void deallocate(void *p) noexcept override {
    base_t::deallocate(p);
  }

public:
  static block_pool_resource *get() noexcept;

  using base_t::base_t;

  void *allocate(std::size_t /*bytes*/, std::size_t /*alignment*/ = alignof(std::max_align_t)) noexcept {
    void *p = base_t::allocate();
    p = ::LIBIMP::construct<std::size_t>(p, BlockSize);
    return reinterpret_cast<::LIBIMP::byte *>(p) + regular_head_size;
  }

  void deallocate(void *p, std::size_t /*bytes*/, std::size_t /*alignment*/ = alignof(std::max_align_t)) noexcept {
    p = reinterpret_cast<::LIBIMP::byte *>(p) - regular_head_size;
    auto r_size = *static_cast<std::size_t *>(p);
    if (r_size <= BlockSize) {
      base_t::deallocate(p);
      return;
    }
    // When the actual size exceeds the current memory block size, 
    // try to find a suitable pool among all memory block pools for this thread.
    auto &map = get_thread_block_pool_map();
    auto it = map.find(r_size);
    if ((it == map.end()) || (it->second == nullptr)) {
      block_pool_resource<0, 0> *bp = nullptr;
      LIBIMP_TRY {
        // If the corresponding memory resource cannot be found, 
        // create a temporary general-purpose block pool to deallocate memory.
        it = map.emplace(r_size, bp = new block_pool_resource<0, 0>).first;
      } LIBIMP_CATCH(...) {
        // If the memory resource cannot be created, 
        // store the pointer directly to avoid leakage.
        delete bp;
        base_t::deallocate(p);
        return;
      }
    }
    it->second->deallocate(p);
  }
};

template <std::size_t BlockSize, std::size_t BlockPoolExpansion>
auto block_pool_resource<BlockSize, BlockPoolExpansion>::get() noexcept
  -> block_pool_resource<BlockSize, BlockPoolExpansion> * {
  thread_local block_pool_resource *pi = nullptr;
  if (pi != nullptr) {
    return pi;
  }
  // Create a new block pool resource for this thread.
  auto &map = get_thread_block_pool_map();
  auto it = map.find(BlockSize);
  if ((it != map.end()) && (it->second != nullptr)) {
    // If there are existing block pool resources in the thread cache, 
    // a new block pool resource is constructed based on it and the cache is updated.
    auto *bp = static_cast <block_pool<0, 0> *>(
               dynamic_cast<block_pool_resource<0, 0> *>(it->second));
    if (bp == nullptr) {
      return nullptr;
    }
    thread_local block_pool_resource instance(std::move(*bp));
    delete static_cast<block_pool_resource<0, 0> *>(bp);
    it->second = pi = &instance;
    return pi;
  } else {
    // If there are no existing block pool resources in the thread cache, 
    // the thread local storage instance is constructed and the pointer is cached.
    thread_local block_pool_resource instance;
    LIBIMP_TRY {
      map.emplace(BlockSize, pi = &instance);
      return pi;
    } LIBIMP_CATCH(...) {
      return nullptr;
    }
  }
}

/// \brief Match the appropriate memory block resources 
///        according to the size of the specification.
template <std::size_t N, std::size_t L = regular_level(N)>
class regular_resource : public new_delete_resource {};

/// \brief Different increment levels match different chunk sizes.
///        512 means that 512 consecutive memory blocks are allocated at a time, and the block size is N.
template <std::size_t N> class regular_resource<N, 0> : public block_pool_resource<N, 512> {};
template <std::size_t N> class regular_resource<N, 1> : public block_pool_resource<N, 256> {};
template <std::size_t N> class regular_resource<N, 2> : public block_pool_resource<N, 128> {};
template <std::size_t N> class regular_resource<N, 3> : public block_pool_resource<N, 64 > {};

/// \brief Creates an object based on the specified type and parameters with block pool resource.
/// \note This function is thread-safe.
template <typename T, typename... A>
T *new$(A &&... args) noexcept {
  auto *mem_res = regular_resource<regular_sizeof<T>()>::get();
  if (mem_res == nullptr) return nullptr;
  return ::LIBIMP::construct<T>(mem_res->allocate(sizeof(T), alignof(T)), std::forward<A>(args)...);
}

/// \brief Destroys object previously allocated by the `new$` and releases obtained memory area.
/// \note This function is thread-safe. If the pointer type passed in is different from `new$`, 
///       additional performance penalties may be incurred.
template <typename T>
void delete$(T *p) noexcept {
  if (p == nullptr) return;
  ::LIBIMP::destroy(p);
  auto *mem_res = regular_resource<regular_sizeof<T>()>::get();
  if (mem_res == nullptr) return;
#if defined(LIBIMP_CC_MSVC_2015)
  // `alignof` of vs2015 requires that type must be able to be instantiated.
  mem_res->deallocate(p, sizeof(T));
#else
  mem_res->deallocate(p, sizeof(T), alignof(T));
#endif
}

LIBPMR_NAMESPACE_END_
