/**
 * \file libimp/dataof.h
 * \author mutouyun (orz@orzz.org)
 * \brief Returns the data pointer of the given range
 * \date 2023-05-27
 */
#pragma once

#include <type_traits>  // std::declval, std::true_type, std::false_type
#include <cstddef>      // std::size_t

#include "libimp/def.h"
#include "libimp/generic.h"

LIBIMP_NAMESPACE_BEG_

/**
 * \see https://en.cppreference.com/w/cpp/iterator/data
*/

namespace detail_dataof {

template <typename T>
struct trait_has_data {
private:
  template <typename Type>
  static std::true_type check(decltype(std::declval<Type>().data())*);
  template <typename Type>
  static std::false_type check(...);
public:
  using type = decltype(check<T>(nullptr));
  constexpr static auto value = type::value;
};

template <typename T>
struct trait_has_Data {
private:
  template <typename Type>
  static std::true_type check(decltype(std::declval<Type>().Data())*);
  template <typename Type>
  static std::false_type check(...);
public:
  using type = decltype(check<T>(nullptr));
  constexpr static auto value = type::value;
};

template <typename C, bool = trait_has_data<C>::value
                    , bool = trait_has_Data<C>::value>
struct trait;

template <typename T, std::size_t N>
struct trait<T[N], false, false> {
  constexpr static T const *dataof(T const (&arr)[N]) noexcept {
    return arr;
  }
  constexpr static T *dataof(T (&arr)[N]) noexcept {
    return arr;
  }
};

template <typename C, bool B>
struct trait<C, true, B> {
  template <typename T>
  constexpr static auto dataof(T &&c) noexcept(noexcept(c.data())) {
    return std::forward<T>(c).data();
  }
};

template <typename C>
struct trait<C, false, true> {
  template <typename T>
  constexpr static auto dataof(T &&c) noexcept(noexcept(c.Data())) {
    return std::forward<T>(c).Data();
  }
};

template <typename C>
struct trait<C, false, false> {
  template <typename T>
  constexpr static T const *dataof(std::initializer_list<T> il) noexcept {
    return il.begin();
  }
};

} // namespace detail_dataof

template <typename T, typename R = detail_dataof::trait<std::remove_cv_t<std::remove_reference_t<T>>>>
constexpr auto dataof(T &&c) noexcept(noexcept(R::dataof(std::forward<T>(c)))) {
  return R::dataof(std::forward<T>(c));
}

LIBIMP_NAMESPACE_END_
