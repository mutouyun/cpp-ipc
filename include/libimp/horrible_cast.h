/**
 * \file libimp/horrible_cast.h
 * \author mutouyun (orz@orzz.org)
 * \date 2022-04-17
 */
#pragma once

#include <type_traits>  // std::decay_t
#include <utility>

#include "libimp/def.h"

LIBIMP_NAMESPACE_BEG_

namespace detail_horrible_cast {

template <typename T, typename U>
union temp {
  std::decay_t<U> in;
  T out;
};

} // namespace detail_horrible_cast

template <typename T, typename U>
constexpr T horrible_cast(U &&in) noexcept {
  return detail_horrible_cast::temp<T, U>{std::forward<U>(in)}.out;
}

LIBIMP_NAMESPACE_END_
