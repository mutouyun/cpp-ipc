/**
 * @file src/countof.h
 * @author mutouyun (orz@orzz.org)
 * @brief Returns the size of the given range
 * @date 2022-03-01
 */
#pragma once

#include <cstddef>  // std::size_t

#include "libipc/def.h"

LIBIPC_NAMESPACE_BEG_

/**
 * @see https://en.cppreference.com/w/cpp/iterator/size
*/

template <typename T, std::size_t N>
constexpr std::size_t countof(T const (&)[N]) noexcept {
  return N;
}

template <typename C>
constexpr auto countof(C const &c) -> decltype(c.size()) {
  return c.size();
}

template <typename C>
constexpr auto countof(C const &c) -> decltype(c.Size()) {
  return c.Size();
}

LIBIPC_NAMESPACE_END_
