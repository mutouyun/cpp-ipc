/**
 * \file libpmr/block_pool.h
 * \author mutouyun (orz@orzz.org)
 * \brief Fixed-length memory block pool.
 * \date 2023-11-18
 */
#pragma once

#include <cstddef>
#include <new>
#include <array>
#include <deque>

#include "libimp/byte.h"
#include "libimp/export.h"
#include "libconcur/intrusive_stack.h"

#include "libpmr/def.h"
#include "libpmr/allocator.h"

LIBPMR_NAMESPACE_BEG_

/// \brief Get the central cache allocator.
/// \note The central cache allocator is used to allocate memory for the central cache pool.
///       The underlying memory resource is a `monotonic_buffer_resource` with a fixed-size buffer.
LIBIMP_EXPORT allocator &central_cache_allocator() noexcept;

/**
 * \brief The block type.
 * \tparam BlockSize specifies the memory block size
*/
template <std::size_t BlockSize>
union block {
  block *next;
  alignas(std::max_align_t) std::array<::LIBIMP::byte, BlockSize> storage;
};

/**
 * \brief A fixed-length memory block central cache pool.
 * \tparam BlockSize specifies the memory block size
 */
template <typename BlockT, std::size_t BlockPoolExpansion>
class central_cache_pool {

  /// \brief The block type, which should be a union of a pointer and a storage.
  using block_t = BlockT;
  /// \brief The chunk type, which is an array of blocks.
  using chunk_t = std::array<block_t, BlockPoolExpansion>;
  /// \brief The node type, which is used to store the block pointer.
  using node_t = typename ::LIBCONCUR::intrusive_stack<block_t *>::node;

  /// \brief The central cache stack.
  ::LIBCONCUR::intrusive_stack<block_t *> cached_;
  ::LIBCONCUR::intrusive_stack<block_t *> aqueired_;

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
    for (std::size_t i = 0; i < block_pool_expansion - 1; ++i) {
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

/**
 * \brief Fixed-length memory block pool.
 * \tparam BlockSize specifies the memory block size
 * \tparam BlockPoolExpansion specifies the default number of blocks to expand when the block pool is exhausted
 */
template <std::size_t BlockSize, std::size_t BlockPoolExpansion>
class block_pool {

  /// \brief The block type.
  using block_t = block<BlockSize>;
  /// \brief The central cache pool type.
  using central_cache_pool_t = central_cache_pool<block_t, BlockPoolExpansion>;

  /// \brief Expand the block pool when it is exhausted.
  block_t *expand() noexcept {
    return central_cache_pool_t::instance().aqueire();
  }

public:
  constexpr static std::size_t block_size = BlockSize;

  block_pool() noexcept : cursor_(expand()) {}
  ~block_pool() noexcept {
    central_cache_pool_t::instance().release(cursor_);
  }

  block_pool(block_pool const &) = delete;
  block_pool& operator=(block_pool const &) = delete;

  void *allocate() noexcept {
    if (cursor_ == nullptr) {
      cursor_ = expand();
      if (cursor_ == nullptr) return nullptr;
    }
    block_t *p = cursor_;
    cursor_ = cursor_->next;
    return p->storage.data();
  }

  void deallocate(void *p) noexcept {
    if (p == nullptr) return;
    block_t *b = static_cast<block_t *>(p);
    b->next = cursor_;
    cursor_ = b;
  }

private:
  block_t *cursor_;
};

LIBPMR_NAMESPACE_END_
