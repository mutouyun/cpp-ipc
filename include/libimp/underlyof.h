/**
 * \file libimp/underlyof.h
 * \author mutouyun (orz@orzz.org)
 * \brief Returns the underlying type of the given enum.
 * \date 2022-03-01
 */
#pragma once

#include <type_traits>  // std::underlying_type_t

#include "libimp/def.h"

LIBIMP_NAMESPACE_BEG_

/// \brief Returns after converting the value to the underlying type of E.
/// \see https://en.cppreference.com/w/cpp/types/underlying_type
///      https://en.cppreference.com/w/cpp/utility/to_underlying
template <typename E>
constexpr auto underlyof(E e) noexcept {
  return static_cast<std::underlying_type_t<E>>(e);
}

LIBIMP_NAMESPACE_END_
