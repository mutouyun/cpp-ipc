/**
 * \file libipc/block_pool.h
 * \author mutouyun (orz@orzz.org)
 * \brief The fixed-length memory block pool.
 */
#pragma once

#include <cstddef>

#include "libipc/mem/central_cache_pool.h"

namespace ipc {
namespace mem {

/**
 * \brief Fixed-length memory block pool.
 * \tparam BlockSize specifies the memory block size
 * \tparam BlockPoolExpansion specifies the default number of blocks to expand when the block pool is exhausted
 */
template <std::size_t BlockSize, std::size_t BlockPoolExpansion>
class block_pool;

/// \brief General-purpose block pool for any size of memory block.
/// \note This block pool can only be used to deallocate a group of memory blocks of unknown but consistent size, 
///       and cannot be used for memory block allocation.
template <>
class block_pool<0, 0> {

  template <std::size_t BlockSize, std::size_t BlockPoolExpansion>
  friend class block_pool;

  /// \brief The block type.
  struct block_t {
    block_t *next;
  };

  /// \brief The central cache pool type.
  using central_cache_pool_t = central_cache_pool<block_t, 0>;

public:
  static constexpr std::size_t block_size = 0;

  block_pool() noexcept : cursor_(central_cache_pool_t::instance().aqueire()) {}
  ~block_pool() noexcept {
    central_cache_pool_t::instance().release(cursor_);
  }

  block_pool(block_pool const &) = delete;
  block_pool& operator=(block_pool const &) = delete;

  block_pool(block_pool &&rhs) noexcept : cursor_(std::exchange(rhs.cursor_, nullptr)) {}
  block_pool &operator=(block_pool &&) noexcept = delete;

  void deallocate(void *p) noexcept {
    if (p == nullptr) return;
    block_t *b = static_cast<block_t *>(p);
    b->next = cursor_;
    cursor_ = b;
  }

private:
  block_t *cursor_;
};

/// \brief A block pool for a block of memory of a specific size.
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
  static constexpr std::size_t block_size = BlockSize;

  block_pool() noexcept : cursor_(expand()) {}
  ~block_pool() noexcept {
    central_cache_pool_t::instance().release(cursor_);
  }

  block_pool(block_pool const &) = delete;
  block_pool& operator=(block_pool const &) = delete;

  block_pool(block_pool &&rhs) noexcept
    : cursor_(std::exchange(rhs.cursor_, nullptr)) {}
  block_pool &operator=(block_pool &&) noexcept = delete;

  /// \brief Used to take all memory blocks from within a general-purpose block pool.
  /// \note Of course, the actual memory blocks they manage must be the same size.
  block_pool(block_pool<0, 0> &&rhs) noexcept
    : cursor_(reinterpret_cast<block_t *>(std::exchange(rhs.cursor_, nullptr))) {}

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

} // namespace mem
} // namespace ipc
