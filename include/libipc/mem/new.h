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
#include <tuple>

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
  virtual void recycle(void *p) noexcept = 0;
};

#if defined(LIBIPC_CPP_17)
using recycle_t = void (*)(void *p, void *o) noexcept;
#else
using recycle_t = void (*)(void *p, void *o);
#endif

static constexpr std::size_t regular_head_size
    = round_up(sizeof(recycle_t), alignof(std::max_align_t));

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
constexpr inline std::size_t regular_sizeof(std::size_t s) noexcept {
  return regular_sizeof_impl(regular_head_size + s);
}
template <typename T, std::size_t S = sizeof(T)>
constexpr inline std::size_t regular_sizeof() noexcept {
  return regular_sizeof(S);
}

/// \brief Use block pools to handle memory less than 64K.
template <std::size_t BlockSize, std::size_t BlockPoolExpansion>
class block_resource_base : public block_pool<BlockSize, BlockPoolExpansion> {
public:
  void *allocate(std::size_t /*bytes*/, std::size_t /*alignment*/) noexcept {
    return block_pool<BlockSize, BlockPoolExpansion>::allocate();
  }

  void deallocate(void *p) noexcept {
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

  void deallocate(void *p) noexcept {
    new_delete_resource::deallocate(p, regular_head_size);
  }
};

/// \brief Defines block pool memory resource based on block pool.
template <std::size_t BlockSize, std::size_t BlockPoolExpansion>
class block_pool_resource : public block_resource_base<BlockSize, BlockPoolExpansion>
                          , public block_collector {

  using base_t = block_resource_base<BlockSize, BlockPoolExpansion>;

  void recycle(void *p) noexcept override {
    base_t::deallocate(p);
  }

public:
  static block_collector *get() noexcept {
    thread_local block_pool_resource instance;
    return &instance;
  }

  template <typename T>
  void *allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) noexcept {
    void *b = base_t::allocate(bytes, alignment);
    *static_cast<recycle_t *>(b)
      = [](void *b, void *p) noexcept {
          std::ignore = destroy(static_cast<T *>(p));
          get()->recycle(b);
        };
    return static_cast<byte *>(b) + regular_head_size;
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
template <typename T, std::enable_if_t<std::is_void<T>::value, bool> = true>
auto *get_regular_resource() noexcept {
  using block_poll_resource_t = block_pool_resource<0, 0>;
  return dynamic_cast<block_poll_resource_t *>(block_poll_resource_t::get());
}

namespace detail_new {

template <typename T>
struct do_allocate {
  template <typename R, typename... A>
  static T *apply(R *res, A &&... args) noexcept {
    LIBIPC_TRY {
      return construct<T>(res->template allocate<T>(sizeof(T), alignof(T)), std::forward<A>(args)...);
    } LIBIPC_CATCH(...) {
      return nullptr;
    }
  }
};

template <>
struct do_allocate<void> {
  template <typename R>
  static void *apply(R *res, std::size_t bytes) noexcept {
    if (bytes == 0) return nullptr;
    return res->template allocate<void>(bytes);
  }
};

} // namespace detail_new

/// \brief Creates an object based on the specified type and parameters with block pool resource.
/// \note This function is thread-safe.
template <typename T, typename... A>
T *$new(A &&... args) noexcept {
  auto *res = get_regular_resource<T>();
  if (res == nullptr) return nullptr;
  return detail_new::do_allocate<T>::apply(res, std::forward<A>(args)...);
}

/// \brief Destroys object previously allocated by the `$new` and releases obtained memory area.
/// \note This function is thread-safe. If the pointer type passed in is different from `$new`, 
///       additional performance penalties may be incurred.
template <typename T>
void $delete(T *p) noexcept {
  if (p == nullptr) return;
  auto *b = reinterpret_cast<byte *>(p) - regular_head_size;
  auto *r = reinterpret_cast<recycle_t *>(b);
  (*r)(b, p);
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
