/**
 * @file libimp/fmt_cpo.h
 * @author mutouyun (orz@orzz.org)
 * @brief String formatting CPO.
 * @date 2022-11-28
 */
#pragma once

#include <string>

#include "libimp/def.h"
#include "libimp/generic.h"

LIBIMP_NAMESPACE_BEG_

/**
 * @brief Supports custom fmt_to_string methods for imp::fmt.
 */
namespace detail {

struct fmt_to_string_t {
  template <typename T>
  std::string operator()(T &&arg) const {
    return ::LIBIMP::tag_invoke(fmt_to_string_t{}, std::forward<T>(arg));
  }
};

} // namespace detail

constexpr detail::fmt_to_string_t fmt_to_string {};

LIBIMP_NAMESPACE_END_
