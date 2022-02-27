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

#include "libipc/def.h"

LIBIPC_NAMESPACE_BEG_

template <typename T, typename... A>
auto construct(void *p, A &&... args)
  -> std::enable_if_t<::std::is_constructible<T, A...>::value, T *> {
  return ::new (p) T(std::forward<A>(args)...);
}

template <typename T, typename... A>
auto construct(void *p, A &&... args)
  -> std::enable_if_t<!::std::is_constructible<T, A...>::value, T *> {
  return ::new (p) T{std::forward<A>(args)...};
}

template <typename T>
void *destroy(T *p) noexcept {
  p->~T();
  return p;
}

LIBIPC_NAMESPACE_END_
