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
#include "libconcur/intrusive_stack.h"

#include "libpmr/def.h"
#include "libpmr/memory_resource.h"

LIBPMR_NAMESPACE_BEG_

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
template <std::size_t BlockSize>
class central_cache_pool {

  /// \brief The block type.
  using block_t = block<BlockSize>;

  /// \brief The central cache stack.
  ::LIBCONCUR::intrusive_stack<block_t *> cache_;
  ::LIBCONCUR::intrusive_stack<block_t *> aqueired_;

  central_cache_pool() noexcept = default;

public:
  block_t *aqueire() noexcept {
    auto *n = cache_.pop();
    if (n != nullptr) {
      aqueired_.push(n);
      return n->value;
    }
    auto *blocks = new(std::nothrow) std::array<block_t, block_pool_expansion>;
    if (blocks == nullptr) {
      return nullptr;
    }
    for (std::size_t i = 0; i < block_pool_expansion - 1; ++i) {
      (*blocks)[i].next = &(*blocks)[i + 1];
    }
    blocks->back().next = nullptr;
    return blocks->data();
  }

  void release(block_t *p) noexcept {
    if (p == nullptr) return;
    auto *a = aqueired_.pop();
    if (a == nullptr) {
      a = new(std::nothrow) ::LIBCONCUR::intrusive_stack<block_t *>::node;
      if (a == nullptr) return;
    }
    a->value = p;
    cache_.push(a);
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
 */
template <std::size_t BlockSize>
class block_pool {

  /// \brief The block type.
  using block_t = block<BlockSize>;

  /// \brief Expand the block pool when it is exhausted.
  block_t *expand() noexcept {
    return central_cache_pool<BlockSize>::instance().aqueire();
  }

public:
  constexpr static std::size_t block_size = BlockSize;

  block_pool() noexcept : cursor_(expand()) {}
  ~block_pool() noexcept {
    central_cache_pool<BlockSize>::instance().release(cursor_);
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
