/**
 * \file libipc/central_cache_pool.h
 * \author mutouyun (orz@orzz.org)
 * \brief The fixed-length memory block central cache pool.
 */
#pragma once

#include <cstddef>
#include <deque>
#include <utility>
#include <array>

#include "libipc/imp/byte.h"
#include "libipc/concur/intrusive_stack.h"
#include "libipc/mem/central_cache_allocator.h"

namespace ipc {
namespace mem {

/**
 * \brief The block type.
 * \tparam BlockSize specifies the memory block size
*/
template <std::size_t BlockSize>
union block {
  block *next;
  alignas(std::max_align_t) std::array<byte, BlockSize> storage;
};

/**
 * \brief A fixed-length memory block central cache pool.
 * \tparam BlockT specifies the memory block type
 */
template <typename BlockT, std::size_t BlockPoolExpansion>
class central_cache_pool {

  /// \brief The block type, which should be a union of a pointer and a storage.
  using block_t = BlockT;
  /// \brief The chunk type, which is an array of blocks.
  using chunk_t = std::array<block_t, BlockPoolExpansion>;
  /// \brief The node type, which is used to store the block pointer.
  using node_t  = typename concur::intrusive_stack<block_t *>::node;

  /// \brief The central cache stack.
  concur::intrusive_stack<block_t *> cached_;
  concur::intrusive_stack<block_t *> aqueired_;

  central_cache_pool() noexcept = default;

public:
  block_t *aqueire() noexcept {
    auto *n = cached_.pop();
    if (n != nullptr) {
      aqueired_.push(n);
      return n->value;
    }
    auto *chunk = central_cache_allocator().construct<chunk_t>();
    if (chunk == nullptr) {
      return nullptr;
    }
    for (std::size_t i = 0; i < BlockPoolExpansion - 1; ++i) {
      (*chunk)[i].next = &(*chunk)[i + 1];
    }
    chunk->back().next = nullptr;
    return chunk->data();
  }

  void release(block_t *p) noexcept {
    if (p == nullptr) return;
    auto *a = aqueired_.pop();
    if (a == nullptr) {
      a = central_cache_allocator().construct<node_t>();
      if (a == nullptr) return;
    }
    a->value = p;
    cached_.push(a);
  }

  /// \brief Get the singleton instance.
  static central_cache_pool &instance() noexcept {
    static central_cache_pool pool;
    return pool;
  }
};

/// \brief A fixed-length memory block central cache pool with no default expansion size.
template <typename BlockT>
class central_cache_pool<BlockT, 0> {

  /// \brief The block type, which should be a union of a pointer and a storage.
  using block_t = BlockT;
  /// \brief The node type, which is used to store the block pointer.
  using node_t = typename concur::intrusive_stack<block_t *>::node;

  /// \brief The central cache stack.
  concur::intrusive_stack<block_t *> cached_;
  concur::intrusive_stack<block_t *> aqueired_;

  central_cache_pool() noexcept = default;

public:
  block_t *aqueire() noexcept {
    auto *n = cached_.pop();
    if (n != nullptr) {
      aqueired_.push(n);
      return n->value;
    }
    // For pools with no default expansion size, 
    // the central cache pool is only buffered, not allocated.
    return nullptr;
  }

  void release(block_t *p) noexcept {
    if (p == nullptr) return;
    auto *a = aqueired_.pop();
    if (a == nullptr) {
      a = central_cache_allocator().construct<node_t>();
      if (a == nullptr) return;
    }
    a->value = p;
    cached_.push(a);
  }

  /// \brief Get the singleton instance.
  static central_cache_pool &instance() noexcept {
    static central_cache_pool pool;
    return pool;
  }
};

} // namespace mem
} // namespace ipc
