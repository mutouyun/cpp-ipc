/**
 * @file libimp/span.h
 * @author mutouyun (orz@orzz.org)
 * @brief Describes an object that can refer to a contiguous sequence of objects
 * @date 2022-10-16
 */
#pragma once

#include <type_traits>
#include <cstddef>
#include <iterator>
#include <array>
#include <vector>
#include <string>
#include <limits>
#include <algorithm>
#include <cstdint>
#include <initializer_list>

#include "fmt/format.h"

#include "libimp/def.h"
#include "libimp/detect_plat.h"
#include "libimp/byte.h"

#if defined(LIBIMP_CPP_20) && defined(__cpp_lib_span)
#include <span>
#define LIBIMP_CPP_LIB_SPAN_
#endif // __cpp_lib_span

LIBIMP_NAMESPACE_BEG_
namespace detail {

/// @brief helper trait for span

template <typename From, typename To>
using array_convertible = std::is_convertible<From(*)[], To(*)[]>;

template <typename From, typename To>
using is_array_convertible = 
  typename std::enable_if<array_convertible<From, To>::value>::type;

template <typename T, typename Ref>
using compatible_ref = array_convertible<typename std::remove_reference<Ref>::type, T>;

template <typename It>
using iter_reference_t = decltype(*std::declval<It&>());

template <typename T, typename It>
using is_compatible_iter = 
  typename std::enable_if<compatible_ref<T, iter_reference_t<It>>::value>::type;

template <typename From, typename To>
using is_inconvertible = 
  typename std::enable_if<!std::is_convertible<From, To>::value>::type;

template <typename S, typename I>
using is_sized_sentinel_for = 
  typename std::enable_if<std::is_convertible<decltype(std::declval<S>() - std::declval<I>()), 
                          std::ptrdiff_t>::value>::type;

/// @brief Obtain the address represented by p 
///        without forming a reference to the object pointed to by p.
/// @see https://en.cppreference.com/w/cpp/memory/to_address

template<typename T>
constexpr T *to_address(T *ptr) noexcept {
  static_assert(!std::is_function<T>::value, "ptr shouldn't a function pointer");
  return ptr;
}

template<typename T>
constexpr auto to_address(T const &ptr) 
  noexcept(noexcept(ptr.operator->()))
        -> decltype(ptr.operator->()) {
  return to_address(ptr.operator->());
}

} // namespace detail

/**
 * @brief A simple implementation of span.
 * @see https://en.cppreference.com/w/cpp/container/span
 */
template <typename T>
class span {
public:
  using element_type     = T;
  using value_type       = typename std::remove_cv<element_type>::type;
  using size_type        = std::size_t;
  using difference_type  = std::ptrdiff_t;
  using pointer          = element_type *;
  using const_pointer    = typename std::remove_const<element_type>::type const *;
  using reference        = element_type &;
  using const_reference  = typename std::remove_const<element_type>::type const &;
  using iterator         = pointer;
  using reverse_iterator = std::reverse_iterator<iterator>;

private:
  pointer   ptr_    {nullptr};
  size_type extent_ {0};

public:
  constexpr span() noexcept = default;
  constexpr span(span const &) noexcept = default;
#if (LIBIMP_CC_MSVC > LIBIMP_CC_MSVC_2015)
  constexpr
#endif
  span & operator=(span const &) noexcept = default;

  template <typename It, 
            typename = detail::is_compatible_iter<T, It>>
  constexpr span(It first, size_type count) noexcept
    : ptr_   (detail::to_address(first))
    , extent_(count) {}

  template <typename It, typename End,
            typename = detail::is_compatible_iter<T, It>,
            typename = detail::is_sized_sentinel_for<End, It>,
            typename = detail::is_inconvertible<End, size_type>>
  constexpr span(It first, End last) noexcept(noexcept(last - first))
    : ptr_   (detail::to_address(first))
    , extent_(static_cast<size_type>(last - first)) {}

  template <typename U, std::size_t E,
            typename = detail::is_array_convertible<U, element_type>>
  constexpr span(U (&arr)[E]) noexcept
    : span(static_cast<pointer>(arr), E) {}

  template <typename U, std::size_t E,
            typename = detail::is_array_convertible<U, element_type>>
  constexpr span(std::array<U, E> &arr) noexcept
	  : span(static_cast<pointer>(arr.data()), E) {}

  template <typename U, std::size_t E,
            typename = detail::is_array_convertible<typename std::add_const<U>::type, element_type>>
  constexpr span(std::array<U, E> const &arr) noexcept
	  : span(static_cast<pointer>(arr.data()), E) {}

  template <typename U,
            typename = detail::is_array_convertible<U, T>>
  constexpr span(span<U> const &s) noexcept
    : ptr_   (s.data())
    , extent_(s.size()) {}

#ifdef LIBIMP_CPP_LIB_SPAN_
  template <typename U, std::size_t E,
            typename = detail::is_array_convertible<U, T>>
  constexpr span(std::span<U, E> const &s) noexcept
    : ptr_   (s.data())
    , extent_(s.size()) {}
#endif // LIBIMP_CPP_LIB_SPAN_

  constexpr size_type size() const noexcept {
    return extent_;
  }

  constexpr size_type size_bytes() const noexcept {
    return size() * sizeof(element_type);
  }

  constexpr bool empty() const noexcept {
    return size() == 0;
  }

  constexpr pointer data() const noexcept {
    return this->ptr_;
  }

  constexpr reference front() const noexcept {
    return *data();
  }

  constexpr reference back() const noexcept {
    return *(data() + (size() - 1));
  }

  constexpr reference operator[](size_type idx) const noexcept {
    return *(data() + idx);
  }

  constexpr iterator begin() const noexcept {
    return iterator(data());
  }

  constexpr iterator end() const noexcept {
    return iterator(data() + this->size());
  }

  constexpr reverse_iterator rbegin() const noexcept {
    return reverse_iterator(this->end());
  }

  constexpr reverse_iterator rend() const noexcept {
    return reverse_iterator(this->begin());
  }

  constexpr span first(size_type count) const noexcept {
    return span(begin(), count);
  }

  constexpr span last(size_type count) const noexcept {
    return span(end() - count, count);
  }

  constexpr span subspan(size_type offset, size_type count = (std::numeric_limits<size_type>::max)()) const noexcept {
    return (offset >= size()) ? span() : span(begin() + offset, (std::min)(size() - offset, count));
  }
};

/// @brief Support for span equals comparison.

template <typename T, typename U, 
          typename = decltype(std::declval<T>() == std::declval<U>())>
bool operator==(span<T> a, span<U> b) noexcept {
  if (a.size() != b.size()) {
    return false;
  }
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

/// @brief Converts a span into a view of its underlying bytes.
/// @see https://en.cppreference.com/w/cpp/container/span/as_bytes

template <typename T, 
          typename Byte = typename std::conditional<std::is_const<T>::value, byte const, byte>::type>
auto as_bytes(span<T> s) noexcept -> span<Byte> {
  return {byte_cast(s.data()), s.size_bytes()};
}

/// @brief Constructs an object of type T and wraps it in a span.
/// Before C++17, template argument deduction for class templates was not supported.
/// @see https://en.cppreference.com/w/cpp/language/template_argument_deduction

template <typename T>
auto make_span(T *arr, std::size_t count) noexcept -> span<T> {
  return {arr, count};
}

template <typename T, std::size_t E>
auto make_span(T (&arr)[E]) noexcept -> span<T> {
  return {arr};
}

template <typename T, std::size_t E>
auto make_span(std::array<T, E> const &arr) noexcept -> span<T> {
  return {arr};
}

template <typename T, std::size_t E>
auto make_span(std::array<T, E> const &arr) noexcept -> span<typename std::add_const<T>::type> {
  return {arr};
}

template <typename T>
auto make_span(std::vector<T> &arr) noexcept -> span<T> {
  return {arr.data(), arr.size()};
}

template <typename T>
auto make_span(std::vector<T> const &arr) noexcept -> span<typename std::add_const<T>::type> {
  return {arr.data(), arr.size()};
}

template <typename T>
auto make_span(std::initializer_list<T> list) noexcept -> span<typename std::add_const<T>::type> {
  return {list.begin(), list.end()};
}

inline auto make_span(std::string &str) noexcept -> span<char> {
  return {const_cast<char *>(str.data()), str.size()};
}

inline auto make_span(std::string const &str) noexcept -> span<char const> {
  return {str.data(), str.size()};
}

LIBIMP_NAMESPACE_END_

template <typename T>
struct fmt::formatter<::LIBIMP_::span<T>> {
  constexpr auto parse(format_parse_context& ctx) const {
    return ctx.end();
  }
  template <typename FormatContext>
  auto format(::LIBIMP_::span<T> s, FormatContext &ctx) {
    if (s.empty()) {
      return format_to(ctx.out(), "");
    }
    auto appender = format_to(ctx.out(), "{}", s[0]);
    for (std::size_t i = 1; i < s.size(); ++i) {
      appender = format_to(appender, " {}", s[i]);
    }
    return appender;
  }
};
