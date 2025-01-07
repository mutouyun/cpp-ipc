/**
 * \file libipc/generic.h
 * \author mutouyun (orz@orzz.org)
 * \brief Tools for generic programming.
 */
#pragma once

#include <utility>
#include <type_traits>  // std::declval, std::true_type, std::false_type
#include <cstddef>      // std::size_t

#include "libipc/imp/detect_plat.h"

namespace ipc {

/**
 * \brief Utility metafunction that maps a sequence of any types to the type void
 * \see https://en.cppreference.com/w/cpp/types/void_t
 */
template <typename...>
using void_t = void;

/**
 * \brief A type-list for generic programming.
*/
template <typename...>
struct types {};

/**
 * \brief To indicate that the contained object should be constructed in-place.
 * \see https://en.cppreference.com/w/cpp/utility/in_place
 */
#if defined(LIBIPC_CPP_17)
using std::in_place_t;
using std::in_place;
#else /*!LIBIPC_CPP_17*/
struct in_place_t {
  explicit in_place_t() = default;
};
constexpr in_place_t in_place {};
#endif/*!LIBIPC_CPP_17*/

/**
 * \brief A general pattern for supporting customisable functions
 * \see https://www.open-std.org/jtc1/sc22/WG21/docs/papers/2019/p1895r0.pdf
 */
namespace detail_tag_invoke {

void tag_invoke();

struct tag_invoke_t {
  template <typename T, typename... A>
  constexpr auto operator()(T tag, A &&...args) const
    noexcept(noexcept(tag_invoke(std::forward<T>(tag), std::forward<A>(args)...)))
          -> decltype(tag_invoke(std::forward<T>(tag), std::forward<A>(args)...)) {
    return tag_invoke(std::forward<T>(tag), std::forward<A>(args)...);
  }
};

} // namespace detail_tag_invoke

constexpr detail_tag_invoke::tag_invoke_t tag_invoke{};

/**
 * \brief Circumventing forwarding reference may override copy and move constructs.
 * \see https://mpark.github.io/programming/2014/06/07/beware-of-perfect-forwarding-constructors/
 */
namespace detail_not_match {

template <typename T, typename... A>
struct is_same_first : std::false_type {};

template <typename T>
struct is_same_first<T, T> : std::true_type {};

} // namespace detail_not_match

template <typename T, typename... A>
using not_match = 
  typename std::enable_if<!detail_not_match::is_same_first<T, 
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

/**
 * \brief Returns the size of the given range.
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
  static constexpr auto value = type::value;
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
  static constexpr auto value = type::value;
};

template <typename C, bool = trait_has_size<C>::value
                    , bool = trait_has_Size<C>::value>
struct trait;

template <typename T, std::size_t N>
struct trait<T[N], false, false> {
  static constexpr auto countof(T const (&)[N]) noexcept {
    return N;
  }
};

template <typename C, bool B>
struct trait<C, true, B> {
  static constexpr auto countof(C const &c) noexcept(noexcept(c.size())) {
    return c.size();
  }
};

template <typename C>
struct trait<C, false, true> {
  static constexpr auto countof(C const &c) noexcept(noexcept(c.Size())) {
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

/**
 * \brief Returns the data pointer of the given range.
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
  static constexpr auto value = type::value;
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
  static constexpr auto value = type::value;
};

template <typename C, bool = trait_has_data<C>::value
                    , bool = trait_has_Data<C>::value>
struct trait;

template <typename T, std::size_t N>
struct trait<T[N], false, false> {
  static constexpr T const *dataof(T const (&arr)[N]) noexcept {
    return arr;
  }
  static constexpr T *dataof(T (&arr)[N]) noexcept {
    return arr;
  }
};

template <typename C, bool B>
struct trait<C, true, B> {
  template <typename T>
  static constexpr auto dataof(T &&c) noexcept(noexcept(c.data())) {
    return std::forward<T>(c).data();
  }
};

template <typename C>
struct trait<C, false, true> {
  template <typename T>
  static constexpr auto dataof(T &&c) noexcept(noexcept(c.Data())) {
    return std::forward<T>(c).Data();
  }
};

template <typename C>
struct trait<C, false, false> {
  template <typename T>
  static constexpr T const *dataof(std::initializer_list<T> il) noexcept {
    return il.begin();
  }
  template <typename T>
  static constexpr T const *dataof(T const *p) noexcept {
    return p;
  }
};

} // namespace detail_dataof

template <typename C, 
          typename T = detail_dataof::trait<std::remove_cv_t<std::remove_reference_t<C>>>, 
          typename R = decltype(T::dataof(std::declval<C>()))>
constexpr R dataof(C &&c) noexcept(noexcept(T::dataof(std::forward<C>(c)))) {
  return T::dataof(std::forward<C>(c));
}

/// \brief Returns after converting the value to the underlying type of E.
/// \see https://en.cppreference.com/w/cpp/types/underlying_type
///      https://en.cppreference.com/w/cpp/utility/to_underlying
template <typename E>
constexpr auto underlyof(E e) noexcept {
  return static_cast<std::underlying_type_t<E>>(e);
}

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

} // namespace ipc
