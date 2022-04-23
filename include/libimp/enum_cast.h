/**
 * @file libimp/enum_cast.h
 * @author mutouyun (orz@orzz.org)
 * @brief Returns the underlying type of the given enum
 * @date 2022-03-01
 */
#pragma once

#include <type_traits>  // std::underlying_type_t

#include "libimp/def.h"

LIBIMP_NAMESPACE_BEG_

template <typename E>
constexpr auto enum_cast(E e) noexcept {
  return static_cast<std::underlying_type_t<E>>(e);
}

LIBIMP_NAMESPACE_END_
