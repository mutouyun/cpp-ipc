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

#include "libpmr/def.h"
#include "libpmr/block_pool.h"
#include "libpmr/memory_resource.h"

LIBPMR_NAMESPACE_BEG_

constexpr inline std::size_t regular_level(std::size_t s) noexcept {
  return (s <= 128  ) ? 0 :
         (s <= 1024 ) ? 1 :
         (s <= 8192 ) ? 2 :
         (s <= 65536) ? 3 : 4;
}

constexpr inline std::size_t regular_sizeof_impl(std::size_t l, std::size_t s) noexcept {
  return (l == 0) ? std::max<std::size_t>(::LIBIMP::round_up<std::size_t>(s, 8), regular_head_size) :
         (l == 1) ? ::LIBIMP::round_up<std::size_t>(s, 128) :
         (l == 2) ? ::LIBIMP::round_up<std::size_t>(s, 1024) :
         (l == 3) ? ::LIBIMP::round_up<std::size_t>(s, 8192) : (std::numeric_limits<std::size_t>::max)();
}

constexpr inline std::size_t regular_sizeof(std::size_t s) noexcept {
  return regular_sizeof_impl(regular_level(s), s);
}

template <typename T>
constexpr inline std::size_t regular_sizeof() noexcept {
  return regular_sizeof(regular_head_size + sizeof(T));
}

class block_pool_like {
 public:
  virtual ~block_pool_like() noexcept = default;
  virtual void *allocate() noexcept = 0;
  virtual void deallocate(void *p) noexcept = 0;
};

inline std::unordered_map<std::size_t, block_pool_like *> &get_block_pool_map() noexcept {
  thread_local std::unordered_map<std::size_t, block_pool_like *> instances;
  return instances;
}

template <std::size_t BlockSize, std::size_t BlockPoolExpansion>
class block_pool_resource;

template <>
class block_pool_resource<0, 0> : public block_pool<0, 0>
                                , public block_pool_like {

  void *allocate() noexcept override {
    return nullptr;
  }

public:
  void deallocate(void *p) noexcept override {
    block_pool<0, 0>::deallocate(p);
  }
};

template <std::size_t BlockSize, std::size_t BlockPoolExpansion>
class block_pool_resource : public block_pool<BlockSize, BlockPoolExpansion>
                          , public block_pool_like {

  using base_t = block_pool<BlockSize, BlockPoolExpansion>;

  void *allocate() noexcept override {
    return base_t::allocate();
  }

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
    auto &map = get_block_pool_map();
    auto it = map.find(r_size);
    if ((it == map.end()) || (it->second == nullptr)) LIBIMP_TRY {
      // If the corresponding memory resource cannot be found, 
      // create a temporary general-purpose block pool to deallocate memory.
      it = map.emplace(r_size, new block_pool_resource<0, 0>).first;
    } LIBIMP_CATCH(...) {
      // If the memory resource cannot be created, 
      // store the pointer directly to avoid leakage.
      base_t::deallocate(p);
      return;
    }
    it->second->deallocate(p);
  }
};

template <std::size_t BlockSize, std::size_t BlockPoolExpansion>
auto block_pool_resource<BlockSize, BlockPoolExpansion>::get() noexcept
  -> block_pool_resource<BlockSize, BlockPoolExpansion> * {
  auto &map = get_block_pool_map();
  thread_local block_pool_resource *pi = nullptr;
  if (pi != nullptr) {
    return pi;
  }
  auto it = map.find(BlockSize);
  if ((it != map.end()) && (it->second != nullptr)) {
    auto *bp = static_cast <block_pool<0, 0> *>(
               dynamic_cast<block_pool_resource<0, 0> *>(it->second));
    if (bp == nullptr) {
      return nullptr;
    }
    thread_local block_pool_resource instance(std::move(*bp));
    delete static_cast<block_pool_resource<0, 0> *>(bp);
    pi = &instance;
  } else {
    thread_local block_pool_resource instance;
    pi = &instance;
  }
  LIBIMP_TRY {
    map.emplace(BlockSize, pi);
  } LIBIMP_CATCH(...) {
    return nullptr;
  }
  return pi;
}

template <std::size_t N, std::size_t L = regular_level(N)>
class regular_resource : public new_delete_resource {};

template <std::size_t N>
class regular_resource<N, 0> : public block_pool_resource<N, 512> {};

template <std::size_t N>
class regular_resource<N, 1> : public block_pool_resource<N, 256> {};

template <std::size_t N>
class regular_resource<N, 2> : public block_pool_resource<N, 128> {};

template <std::size_t N>
class regular_resource<N, 3> : public block_pool_resource<N, 64> {};

template <typename T, typename... A>
T *new$(A &&... args) noexcept {
  auto *mem_res = regular_resource<regular_sizeof<T>()>::get();
  if (mem_res == nullptr) return nullptr;
  return ::LIBIMP::construct<T>(mem_res->allocate(sizeof(T), alignof(T)), std::forward<A>(args)...);
}

template <typename T>
void delete$(T *p) noexcept {
  if (p == nullptr) return;
  ::LIBIMP::destroy(p);
  auto *mem_res = regular_resource<regular_sizeof<T>()>::get();
  if (mem_res == nullptr) return;
#if defined(LIBIMP_CC_MSVC_2015)
  mem_res->deallocate(p, sizeof(T));
#else
  mem_res->deallocate(p, sizeof(T), alignof(T));
#endif
}

LIBPMR_NAMESPACE_END_
