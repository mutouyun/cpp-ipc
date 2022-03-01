/**
 * @file src/countof.h
 * @author mutouyun (orz@orzz.org)
 * @brief Returns the size of the given range
 * @date 2022-03-01
 */
#pragma once

#include <type_traits>  // std::declval, std::true_type, std::false_type
#include <cstddef>      // std::size_t

#include "libipc/def.h"
#include "libipc/utility/generic.h"

LIBIPC_NAMESPACE_BEG_

/**
 * @see https://en.cppreference.com/w/cpp/iterator/size
*/

namespace detail {

template <typename T>
struct countof_trait_has_size {
private:
  template <typename Type>
  static std::true_type check(decltype(std::declval<Type>().size())*);
  template <typename Type>
  static std::false_type check(...);
public:
  using type = decltype(check<T>(nullptr));
  constexpr static auto value = type::value;
};

template <typename T>
struct countof_trait_has_Size {
private:
  template <typename Type>
  static std::true_type check(decltype(std::declval<Type>().Size())*);
  template <typename Type>
  static std::false_type check(...);
public:
  using type = decltype(check<T>(nullptr));
  constexpr static auto value = type::value;
};

template <typename C, bool = countof_trait_has_size<C>::value
                    , bool = countof_trait_has_Size<C>::value>
struct countof_trait;

template <typename T, std::size_t N>
struct countof_trait<T[N], false, false> {
  constexpr static auto countof(T const (&)[N]) noexcept {
    return N;
  }
};

template <typename C, bool B>
struct countof_trait<C, true, B> {
  constexpr static auto countof(C const &c) noexcept(noexcept(c.size())) {
    return c.size();
  }
};

template <typename C>
struct countof_trait<C, false, true> {
  constexpr static auto countof(C const &c) noexcept(noexcept(c.Size())) {
    return c.Size();
  }
};

} // namespace detail

template <typename C, typename R = detail::countof_trait<C>>
constexpr auto countof(C const &c) noexcept(noexcept(R::countof(c))) {
  return R::countof(c);
}

LIBIPC_NAMESPACE_END_
