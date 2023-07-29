/**
 * \file libimp/countof.h
 * \author mutouyun (orz@orzz.org)
 * \brief Returns the size of the given range
 * \date 2022-03-01
 */
#pragma once

#include <type_traits>  // std::declval, std::true_type, std::false_type
#include <cstddef>      // std::size_t

#include "libimp/def.h"
#include "libimp/generic.h"

LIBIMP_NAMESPACE_BEG_

/**
 * \see https://en.cppreference.com/w/cpp/iterator/size
*/

namespace detail_countof {

template <typename T>
struct trait_has_size {
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
struct trait_has_Size {
private:
  template <typename Type>
  static std::true_type check(decltype(std::declval<Type>().Size())*);
  template <typename Type>
  static std::false_type check(...);
public:
  using type = decltype(check<T>(nullptr));
  constexpr static auto value = type::value;
};

template <typename C, bool = trait_has_size<C>::value
                    , bool = trait_has_Size<C>::value>
struct trait;

template <typename T, std::size_t N>
struct trait<T[N], false, false> {
  constexpr static auto countof(T const (&)[N]) noexcept {
    return N;
  }
};

template <typename C, bool B>
struct trait<C, true, B> {
  constexpr static auto countof(C const &c) noexcept(noexcept(c.size())) {
    return c.size();
  }
};

template <typename C>
struct trait<C, false, true> {
  constexpr static auto countof(C const &c) noexcept(noexcept(c.Size())) {
    return c.Size();
  }
};

} // namespace detail_countof

template <typename C, 
          typename T = detail_countof::trait<C>, 
          typename R = decltype(T::countof(std::declval<C const &>()))>
constexpr R countof(C const &c) noexcept(noexcept(T::countof(c))) {
  return T::countof(c);
}

LIBIMP_NAMESPACE_END_
