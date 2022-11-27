/**
 * @file libimp/generic.h
 * @author mutouyun (orz@orzz.org)
 * @brief Tools for generic programming.
 * @date 2022-03-01
 */
#pragma once

#include <utility>

#include "libimp/def.h"

LIBIMP_NAMESPACE_BEG_

/**
 * @brief Utility metafunction that maps a sequence of any types to the type void
 * @see https://en.cppreference.com/w/cpp/types/void_t
*/
template <typename...>
using void_t = void;

/**
 * @brief A general pattern for supporting customisable functions
 * @see https://www.open-std.org/jtc1/sc22/WG21/docs/papers/2019/p1895r0.pdf
*/
namespace detail {

void tag_invoke();

struct tag_invoke_t {
  template <typename T, typename... A>
  constexpr auto operator()(T tag, A &&...args) const
    noexcept(noexcept(tag_invoke(std::forward<T>(tag), std::forward<A>(args)...)))
          -> decltype(tag_invoke(std::forward<T>(tag), std::forward<A>(args)...)) {
    return tag_invoke(std::forward<T>(tag), std::forward<A>(args)...);
  }
};

} // namespace detail

constexpr detail::tag_invoke_t tag_invoke {};

LIBIMP_NAMESPACE_END_
