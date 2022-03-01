/**
 * @file src/construct.h
 * @author mutouyun (orz@orzz.org)
 * @brief Construct an object from a memory buffer
 * @date 2022-02-27
 */
#pragma once

#include <new>          // placement-new
#include <type_traits>  // std::enable_if_t
#include <utility>      // std::forward
#include <memory>       // std::construct_at, std::destroy_at, std::addressof
#include <cstddef>

#include "libipc/def.h"
#include "libipc/detect_plat.h"

LIBIPC_NAMESPACE_BEG_

/**
 * @brief Creates an object at a given address, like 'construct_at' in c++20
 * @see https://en.cppreference.com/w/cpp/memory/construct_at
*/

template <typename T, typename... A>
auto construct(void *p, A &&... args)
  -> std::enable_if_t<::std::is_constructible<T, A...>::value, T *> {
#if defined(LIBIPC_CPP_20)
  return std::construct_at(static_cast<T *>(p), std::forward<A>(args)...);
#else
  return ::new (p) T(std::forward<A>(args)...);
#endif
}

template <typename T, typename... A>
auto construct(void *p, A &&... args)
  -> std::enable_if_t<!::std::is_constructible<T, A...>::value, T *> {
  return ::new (p) T{std::forward<A>(args)...};
}

/**
 * @brief Destroys an object at a given address, like 'destroy_at' in c++17
 * @see https://en.cppreference.com/w/cpp/memory/destroy_at
*/

template <typename T>
void *destroy(T *p) noexcept {
#if defined(LIBIPC_CPP_17) && !defined(LIBIPC_CC_GNUC)
  std::destroy_at(p);
#else
  p->~T();
#endif
  return p;
}

template <typename T, std::size_t N>
void *destroy(T (*p)[N]) noexcept {
#if defined(LIBIPC_CPP_20)
  std::destroy_at(p);
#elif defined(LIBIPC_CPP_17) && !defined(LIBIPC_CC_GNUC)
  std::destroy(std::begin(*p), std::end(*p));
#else
  for (auto &elem : *p) destroy(std::addressof(elem));
#endif
  return p;
}

LIBIPC_NAMESPACE_END_
