/**
 * @file src/horrible_cast.h
 * @author mutouyun (orz@orzz.org)
 * @date 2022-04-17
 */
#pragma once

#include <type_traits>  // std::decay_t
#include <utility>

#include "libipc/def.h"

LIBIPC_NAMESPACE_BEG_

template <typename T, typename U>
constexpr auto horrible_cast(U &&in) noexcept
  -> std::enable_if_t<(sizeof(T) == sizeof(std::decay_t<U>)), T> {
  union {
    std::decay_t<U> in;
    T out;
  } u {std::forward<U>(in)};
  return u.out;
}

LIBIPC_NAMESPACE_END_
