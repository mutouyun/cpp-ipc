/**
 * \file libpmr/new.h
 * \author mutouyun (orz@orzz.org)
 * \brief Global object creation function.
 * \date 2024-01-01
 */
#pragma once

#include <cstddef>
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

/// \brief Defines the memory block collector interface.
class LIBIMP_EXPORT block_collector {
public:
  virtual ~block_collector() noexcept = default;
  virtual void deallocate(void *p) noexcept = 0;
};

using get_block_collector_t = block_collector *(*)() noexcept;

static constexpr std::size_t regular_head_size
    = ::LIBIMP::round_up(sizeof(get_block_collector_t), alignof(std::max_align_t));

/// \brief Select the incremental level based on the size.
constexpr inline std::size_t regular_level(std::size_t s) noexcept {
  return (s <= 128  ) ? 0 :
         (s <= 1024 ) ? 1 :
         (s <= 8192 ) ? 2 :
         (s <= 65536) ? 3 : 4;
}

/// \brief Calculates the appropriate memory block size based on the increment level and size.
constexpr inline std::size_t regular_sizeof_impl(std::size_t l, std::size_t s) noexcept {
  return (l == 0) ? ::LIBIMP::round_up<std::size_t>(s, regular_head_size) :
         (l == 1) ? ::LIBIMP::round_up<std::size_t>(s, 128 ) :
         (l == 2) ? ::LIBIMP::round_up<std::size_t>(s, 1024) :
         (l == 3) ? ::LIBIMP::round_up<std::size_t>(s, 8192) : (std::numeric_limits<std::size_t>::max)();
}

/// \brief Calculates the appropriate memory block size based on the size.
constexpr inline std::size_t regular_sizeof_impl(std::size_t s) noexcept {
  return regular_sizeof_impl(regular_level(s), s);
}

/// \brief Calculates the appropriate memory block size based on the specific type.
template <typename T>
constexpr inline std::size_t regular_sizeof() noexcept {
  return regular_sizeof_impl(regular_head_size + sizeof(T));
}

/// \brief Defines block pool memory resource based on block pool.
template <std::size_t BlockSize, std::size_t BlockPoolExpansion>
class block_pool_resource : public block_pool<BlockSize, BlockPoolExpansion>
                          , public block_collector {

  using base_t = block_pool<BlockSize, BlockPoolExpansion>;

  void deallocate(void *p) noexcept override {
    base_t::deallocate(p);
  }

public:
  static block_collector *get() noexcept {
    thread_local block_pool_resource instance;
    return &instance;
  }

  using base_t::base_t;

  void *allocate(std::size_t /*bytes*/, std::size_t /*alignment*/ = alignof(std::max_align_t)) noexcept {
    void *p = base_t::allocate();
    *static_cast<get_block_collector_t *>(p) = get;
    return static_cast<::LIBIMP::byte *>(p) + regular_head_size;
  }

  void deallocate(void *p, std::size_t /*bytes*/, std::size_t /*alignment*/ = alignof(std::max_align_t)) noexcept {
    p = static_cast<::LIBIMP::byte *>(p) - regular_head_size;
    auto g = *static_cast<get_block_collector_t *>(p);
    if (g == get) {
      base_t::deallocate(p);
      return;
    }
    g()->deallocate(p);
  }
};

/// \brief Different increment levels match different chunk sizes.
///        512 means that 512 consecutive memory blocks are allocated at a time, and the block size is N.
template <std::size_t L>
constexpr static std::size_t block_pool_expansion = 0;

template <> constexpr static std::size_t block_pool_expansion<0> = 512;
template <> constexpr static std::size_t block_pool_expansion<1> = 256;
template <> constexpr static std::size_t block_pool_expansion<2> = 128;
template <> constexpr static std::size_t block_pool_expansion<3> = 64;

/// \brief Match the appropriate memory block resources according to the size of the specification.
template <std::size_t N, std::size_t L = regular_level(N)>
struct regular_resource {
  static auto *get() noexcept {
    using block_poll_resource_t = block_pool_resource<N, block_pool_expansion<L>>;
    return dynamic_cast<block_poll_resource_t *>(block_poll_resource_t::get());
  }
};

template <std::size_t N>
struct regular_resource<N, 4> : new_delete_resource {};

/// \brief Creates an object based on the specified type and parameters with block pool resource.
/// \note This function is thread-safe.
template <typename T, typename... A>
T *new$(A &&... args) noexcept {
  auto *res = regular_resource<regular_sizeof<T>()>::get();
  if (res == nullptr) return nullptr;
  return ::LIBIMP::construct<T>(res->allocate(sizeof(T), alignof(T)), std::forward<A>(args)...);
}

/// \brief Destroys object previously allocated by the `new$` and releases obtained memory area.
/// \note This function is thread-safe. If the pointer type passed in is different from `new$`, 
///       additional performance penalties may be incurred.
template <typename T>
void delete$(T *p) noexcept {
  if (p == nullptr) return;
  ::LIBIMP::destroy(p);
  auto *res = regular_resource<regular_sizeof<T>()>::get();
  if (res == nullptr) return;
#if defined(LIBIMP_CC_MSVC_2015)
  // `alignof` of vs2015 requires that type must be able to be instantiated.
  res->deallocate(p, sizeof(T));
#else
  res->deallocate(p, sizeof(T), alignof(T));
#endif
}

LIBPMR_NAMESPACE_END_
