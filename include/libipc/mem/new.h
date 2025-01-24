/**
 * \file libipc/mem.h
 * \author mutouyun (orz@orzz.org)
 * \brief Global memory management.
 */
#pragma once

#include <cstddef>
#include <algorithm>
#include <type_traits>
#include <limits>

#include "libipc/imp/aligned.h"
#include "libipc/imp/uninitialized.h"
#include "libipc/imp/byte.h"
#include "libipc/imp/detect_plat.h"
#include "libipc/imp/export.h"
#include "libipc/mem/memory_resource.h"
#include "libipc/mem/block_pool.h"

namespace ipc {
namespace mem {

/// \brief Defines the memory block collector interface.
class LIBIPC_EXPORT block_collector {
public:
  virtual ~block_collector() noexcept = default;
  virtual void recycle(void *p, std::size_t bytes, std::size_t alignment) noexcept = 0;
};

#if defined(LIBIPC_CPP_17)
using get_block_collector_t = block_collector *(*)() noexcept;
#else
using get_block_collector_t = block_collector *(*)();
#endif

static constexpr std::size_t regular_head_size
    = round_up(sizeof(get_block_collector_t), alignof(std::max_align_t));

/// \brief Select the incremental level based on the size.
constexpr inline std::size_t regular_level(std::size_t s) noexcept {
  return (s <= 128  ) ? 0 :
         (s <= 1024 ) ? 1 :
         (s <= 8192 ) ? 2 :
         (s <= 65536) ? 3 : 4;
}

/// \brief Calculates the appropriate memory block size based on the increment level and size.
constexpr inline std::size_t regular_sizeof_impl(std::size_t l, std::size_t s) noexcept {
  return (l == 0) ? round_up<std::size_t>(s, regular_head_size) :
         (l == 1) ? round_up<std::size_t>(s, 128 ) :
         (l == 2) ? round_up<std::size_t>(s, 1024) :
         (l == 3) ? round_up<std::size_t>(s, 8192) : (std::numeric_limits<std::size_t>::max)();
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

/// \brief Use block pools to handle memory less than 64K.
template <std::size_t BlockSize, std::size_t BlockPoolExpansion>
class block_resource_base : public block_pool<BlockSize, BlockPoolExpansion> {
public:
  void *allocate(std::size_t /*bytes*/, std::size_t /*alignment*/) noexcept {
    return block_pool<BlockSize, BlockPoolExpansion>::allocate();
  }

  void deallocate(void *p, std::size_t /*bytes*/, std::size_t /*alignment*/) noexcept {
    block_pool<BlockSize, BlockPoolExpansion>::deallocate(p);
  }
};

/// \brief Use `new`/`delete` to handle memory larger than 64K.
template <std::size_t BlockSize>
class block_resource_base<BlockSize, 0> : public new_delete_resource {
public:
  void *allocate(std::size_t bytes, std::size_t alignment) noexcept {
    return new_delete_resource::allocate(regular_head_size + bytes, alignment);
  }

  void deallocate(void *p, std::size_t bytes, std::size_t alignment) noexcept {
    new_delete_resource::deallocate(p, regular_head_size + bytes, alignment);
  }
};

/// \brief Defines block pool memory resource based on block pool.
template <std::size_t BlockSize, std::size_t BlockPoolExpansion>
class block_pool_resource : public block_resource_base<BlockSize, BlockPoolExpansion>
                          , public block_collector {

  using base_t = block_resource_base<BlockSize, BlockPoolExpansion>;

  void recycle(void *p, std::size_t bytes, std::size_t alignment) noexcept override {
    base_t::deallocate(p, bytes, alignment);
  }

public:
  static block_collector *get() noexcept {
    thread_local block_pool_resource instance;
    return &instance;
  }

  void *allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) noexcept {
    void *p = base_t::allocate(bytes, alignment);
    *static_cast<get_block_collector_t *>(p) = get;
    return static_cast<byte *>(p) + regular_head_size;
  }

  void deallocate(void *p, std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) noexcept {
    p = static_cast<byte *>(p) - regular_head_size;
    auto g = *static_cast<get_block_collector_t *>(p);
    if (g == get) {
      base_t::deallocate(p, bytes, alignment);
      return;
    }
    g()->recycle(p, bytes, alignment);
  }
};

/// \brief Different increment levels match different chunk sizes.
///        512 means that 512 consecutive memory blocks are allocated at a time.
template <std::size_t L>
constexpr std::size_t block_pool_expansion = 0;

template <> constexpr std::size_t block_pool_expansion<0> = 512;
template <> constexpr std::size_t block_pool_expansion<1> = 256;
template <> constexpr std::size_t block_pool_expansion<2> = 128;
template <> constexpr std::size_t block_pool_expansion<3> = 64;

/// \brief Matches the appropriate memory block resource based on the specified type.
template <typename T, std::size_t N = regular_sizeof<T>(), std::size_t L = regular_level(N)>
auto *get_regular_resource() noexcept {
  using block_poll_resource_t = block_pool_resource<N, block_pool_expansion<L>>;
  return dynamic_cast<block_poll_resource_t *>(block_poll_resource_t::get());
}

/// \brief Creates an object based on the specified type and parameters with block pool resource.
/// \note This function is thread-safe.
template <typename T, typename... A>
T *$new(A &&... args) noexcept {
  auto *res = get_regular_resource<T>();
  if (res == nullptr) return nullptr;
  return construct<T>(res->allocate(sizeof(T), alignof(T)), std::forward<A>(args)...);
}

/// \brief Destroys object previously allocated by the `$new` and releases obtained memory area.
/// \note This function is thread-safe. If the pointer type passed in is different from `$new`, 
///       additional performance penalties may be incurred.
template <typename T>
void $delete(T *p) noexcept {
  if (p == nullptr) return;
  destroy(p);
  auto *res = get_regular_resource<T>();
  if (res == nullptr) return;
#if (LIBIPC_CC_MSVC > LIBIPC_CC_MSVC_2015)
  res->deallocate(p, sizeof(T), alignof(T));
#else
  // `alignof` of vs2015 requires that type must be able to be instantiated.
  res->deallocate(p, sizeof(T));
#endif
}

/// \brief The destruction policy used by std::unique_ptr.
/// \see https://en.cppreference.com/w/cpp/memory/default_delete
struct deleter {
  template <typename T>
  void operator()(T *p) const noexcept {
    $delete(p);
  }
};

} // namespace mem
} // namespace ipc
