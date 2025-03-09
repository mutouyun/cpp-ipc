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
  virtual void *allocate(std::size_t /*bytes*/) noexcept = 0;
  virtual void deallocate(void */*p*/, std::size_t /*bytes*/) noexcept = 0;
};

/// \brief Matches the appropriate memory block resource based on a specified size.
LIBIPC_EXPORT block_collector &get_regular_resource(std::size_t s) noexcept;

/// \brief Allocates storage with a size of at least bytes bytes.
LIBIPC_EXPORT void *alloc(std::size_t bytes) noexcept;
LIBIPC_EXPORT void  free (void *p, std::size_t bytes) noexcept;

namespace detail_new {

#if defined(LIBIPC_CPP_17)
using recycle_t = void (*)(void *p) noexcept;
#else
using recycle_t = void (*)(void *p);
#endif

static constexpr std::size_t recycler_size     = round_up(sizeof(recycle_t), alignof(std::size_t));
static constexpr std::size_t allocated_size    = sizeof(std::size_t);
static constexpr std::size_t regular_head_size = round_up(recycler_size + allocated_size, alignof(std::max_align_t));

template <typename T>
struct do_allocate {
  template <typename... A>
  static T *apply(A &&... args) noexcept {
    void *b = mem::alloc(regular_head_size + sizeof(T));
    auto *p = static_cast<byte *>(b) + regular_head_size;
    LIBIPC_TRY {
      T *t = construct<T>(p, std::forward<A>(args)...);
      *reinterpret_cast<recycle_t *>(b)
        = [](void *p) noexcept {
            mem::free(static_cast<byte *>(destroy(static_cast<T *>(p))) - regular_head_size
                    , regular_head_size + sizeof(T));
          };
      return t;
    } LIBIPC_CATCH(...) {
      return nullptr;
    }
  }
};

template <>
struct do_allocate<void> {
  static void *apply(std::size_t bytes) noexcept {
    if (bytes == 0) return nullptr;
    std::size_t rbz = regular_head_size + bytes;
    void *b = mem::alloc(rbz);
    *reinterpret_cast<recycle_t *>(b)
      = [](void *p) noexcept {
          auto *b = static_cast<byte *>(p) - regular_head_size;
          mem::free(b, *reinterpret_cast<std::size_t *>(b + recycler_size));
        };
    auto *z = static_cast<byte *>(b) + recycler_size;
    *reinterpret_cast<std::size_t *>(z) = rbz;
    return static_cast<byte *>(b) + regular_head_size;
  }
};

} // namespace detail_new

/// \brief Creates an object based on the specified type and parameters with block pool resource.
/// \note This function is thread-safe.
template <typename T, typename... A>
T *$new(A &&... args) noexcept {
  return detail_new::do_allocate<T>::apply(std::forward<A>(args)...);
}

/// \brief Destroys object previously allocated by the `$new` and releases obtained memory area.
/// \note This function is thread-safe. If the pointer type passed in is different from `$new`, 
///       additional performance penalties may be incurred.
inline void $delete(void *p) noexcept {
  if (p == nullptr) return;
  auto *r = reinterpret_cast<detail_new::recycle_t *>(static_cast<byte *>(p) - detail_new::regular_head_size);
  (*r)(p);
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
