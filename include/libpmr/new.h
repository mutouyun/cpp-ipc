/**
 * \file libpmr/new.h
 * \author mutouyun (orz@orzz.org)
 * \brief Global object creation function.
 * \date 2024-01-01
 */
#pragma once

#include <cstddef>

#include "libimp/aligned.h"
#include "libimp/uninitialized.h"

#include "libpmr/def.h"
#include "libpmr/block_pool.h"
#include "libpmr/memory_resource.h"

LIBPMR_NAMESPACE_BEG_

constexpr inline std::size_t regular_level(std::size_t s) noexcept {
  if (s <= 128  ) return 0;
  if (s <= 1024 ) return 1;
  if (s <= 8192 ) return 2;
  if (s <= 65536) return 3;
  return 4;
}

constexpr inline std::size_t regular_sizeof(std::size_t s) noexcept {
  switch (regular_level(s)) {
    case 0 : return ::LIBIMP::round_up<std::size_t>(s, 8);
    case 1 : return ::LIBIMP::round_up<std::size_t>(s, 128);
    case 2 : return ::LIBIMP::round_up<std::size_t>(s, 1024);
    case 3 : return ::LIBIMP::round_up<std::size_t>(s, 8192);
    default: return 0;
  }
}

template <typename T>
constexpr inline std::size_t regular_sizeof() noexcept {
  return regular_sizeof(sizeof(T));
}

template <std::size_t BlockSize, std::size_t BlockPoolExpansion>
class block_pool_resource : public block_pool<BlockSize, BlockPoolExpansion> {
public:
  static block_pool_resource *get() noexcept {
    thread_local block_pool_resource instance;
    return &instance;
  }
  void *allocate(std::size_t /*bytes*/, std::size_t /*alignment*/ = alignof(std::max_align_t)) noexcept {
    return block_pool<BlockSize, BlockPoolExpansion>::allocate();
  }
  void deallocate(void *p, std::size_t /*bytes*/, std::size_t /*alignment*/ = alignof(std::max_align_t)) noexcept {
    block_pool<BlockSize, BlockPoolExpansion>::deallocate(p);
  }
};

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
T *new_(A &&... args) noexcept {
  auto *mem_res = regular_resource<regular_sizeof<T>()>::get();
  return ::LIBIMP::construct<T>(mem_res->allocate(sizeof(T), alignof(T)), std::forward<A>(args)...);
}

template <typename T>
void delete_(T *p) noexcept {
  if (p == nullptr) return;
  auto *mem_res = regular_resource<regular_sizeof<T>()>::get();
  mem_res->deallocate(::LIBIMP::destroy(p), sizeof(T), alignof(T));
}

LIBPMR_NAMESPACE_END_
