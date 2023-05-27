/**
 * \file libimp/generic.h
 * \author mutouyun (orz@orzz.org)
 * \brief Tools for generic programming.
 * \date 2022-03-01
 */
#pragma once

#include <utility>
#include <type_traits>

#include "libimp/def.h"
#include "libimp/detect_plat.h"

LIBIMP_NAMESPACE_BEG_

/**
 * \brief Utility metafunction that maps a sequence of any types to the type void
 * \see https://en.cppreference.com/w/cpp/types/void_t
 */
template <typename...>
using void_t = void;

/**
 * \brief To indicate that the contained object should be constructed in-place.
 * \see https://en.cppreference.com/w/cpp/utility/in_place
 */
#if defined(LIBIMP_CPP_17)
using std::in_place_t;
using std::in_place;
#else /*!LIBIMP_CPP_17*/
struct in_place_t {
  explicit in_place_t() = default;
};
constexpr in_place_t in_place {};
#endif/*!LIBIMP_CPP_17*/

/**
 * \brief A general pattern for supporting customisable functions
 * \see https://www.open-std.org/jtc1/sc22/WG21/docs/papers/2019/p1895r0.pdf
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

/**
 * \brief Circumventing forwarding reference may override copy and move constructs.
 * \see https://mpark.github.io/programming/2014/06/07/beware-of-perfect-forwarding-constructors/
 */
namespace detail {

template <typename T, typename... A>
struct is_same_first : std::false_type {};

template <typename T>
struct is_same_first<T, T> : std::true_type {};

} // namespace detail

template <typename T, typename... A>
using not_match = 
  typename std::enable_if<!detail::is_same_first<T, 
  typename std::decay<A>::type...>::value, bool>::type;

/**
 * \brief Determines whether a type is specialized from a particular template.
 */
template <template <typename...> class Tt, typename T>
struct is_specialized : std::false_type {};

template <template <typename...> class Tt, typename... A>
struct is_specialized<Tt, Tt<A...>> : std::true_type {};

/**
 * \brief Copy the cv qualifier and reference of the source type to the target type.
 */
template <typename Src, typename Des>
struct copy_cvref {
  using type = Des;
};

template <typename Src, typename Des>
struct copy_cvref<Src const, Des> {
  using type = typename std::add_const<Des>::type;
};

template <typename Src, typename Des>
struct copy_cvref<Src volatile, Des> {
  using type = typename std::add_volatile<Des>::type;
};

template <typename Src, typename Des>
struct copy_cvref<Src const volatile, Des> {
  using type = typename std::add_cv<Des>::type;
};

template <typename Src, typename Des>
struct copy_cvref<Src &, Des> {
  using type = typename std::add_lvalue_reference<
               typename copy_cvref<Src, Des>::type>::type;
};

template <typename Src, typename Des>
struct copy_cvref<Src &&, Des> {
  using type = typename std::add_rvalue_reference<
               typename copy_cvref<Src, Des>::type>::type;
};

template <typename Src, typename Des>
using copy_cvref_t = typename copy_cvref<Src, Des>::type;

LIBIMP_NAMESPACE_END_
