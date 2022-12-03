/**
 * @file libimp/fmt.h
 * @author mutouyun (orz@orzz.org)
 * @brief String formatting.
 * @date 2022-11-26
 * 
 * @remarks The current performance is not high, 
 * because I use std::sprintf directly for formatting for convenience.
 */
#pragma once

#include <string>
#include <utility>
#include <type_traits>
#include <chrono>   // std::chrono::time_point
#include <cstddef>
#include <ctime>    // std::tm, std::localtime

#include "libimp/def.h"
#include "libimp/fmt_cpo.h"
#include "libimp/span.h"
#include "libimp/detect_plat.h"
#include "libimp/export.h"

LIBIMP_NAMESPACE_BEG_

/**
 * @brief The format string reference wrapper.
 */
template <typename T>
struct fmt_ref {
  span<char const> fstr;
  T param;
};

/**
 * @brief Conversion specifiers.
 * 
 * @remarks Just like printf, the format string is of the form
 * [flags][field_width][.precision][conversion_character]
 * 
 * @see http://personal.ee.surrey.ac.uk/Personal/R.Bowden/C/printf.html
 */
template <std::size_t N>
auto spec(char const (&fstr)[N]) noexcept {
  return [&fstr](auto &&arg) noexcept {
    using arg_t = decltype(arg);
    return fmt_ref<arg_t> {{fstr}, static_cast<arg_t>(arg)};
  };
}

/// @brief String formatting function.

template <typename A>
LIBIMP_NODISCARD std::string fmt(A &&a) {
  return fmt_to_string(std::forward<A>(a));
}

template <typename A1, typename... A>
LIBIMP_NODISCARD std::string fmt(A1 &&a1, A &&...args) {
  std::string joined(fmt(std::forward<A1>(a1)));
  LIBIMP_UNUSED auto unfold = {
    joined.append(fmt_to_string(std::forward<A>(args)))...
  };
  return joined;
}

/// @brief Return the string directly.
inline char const *to_string(char const *a) noexcept { return (a == nullptr) ? "" : a; }
template <std::size_t N>
inline char const *to_string(char const (&a)[N]) noexcept { return a; }
inline std::string to_string(std::string const &a) noexcept { return a; }
inline std::string to_string(std::string &&a) noexcept { return std::move(a); }
LIBIMP_EXPORT std::string to_string(char const *a, span<char const> fstr) noexcept;
inline std::string to_string(std::string const &a, span<char const> fstr) noexcept { return to_string(a.c_str(), fstr); }

/// @brief Character to string conversion.
/// @return an empty string if the conversion fails
inline std::string to_string(char a) noexcept { return {a}; }
LIBIMP_EXPORT std::string to_string(wchar_t a) noexcept;
LIBIMP_EXPORT std::string to_string(char16_t a) noexcept;
LIBIMP_EXPORT std::string to_string(char32_t a) noexcept;
#if defined(LIBIMP_CPP_20)
LIBIMP_EXPORT std::string to_string(char8_t a) noexcept { return to_string((char)a); }
#endif // defined(LIBIMP_CPP_20)

/// @brief Conversion of numeric types to strings.
/// @return an empty string if the conversion fails
LIBIMP_EXPORT std::string to_string(signed short a, span<char const> fstr = {}) noexcept;
LIBIMP_EXPORT std::string to_string(unsigned short a, span<char const> fstr = {}) noexcept;
LIBIMP_EXPORT std::string to_string(signed int a, span<char const> fstr = {}) noexcept;
LIBIMP_EXPORT std::string to_string(unsigned int a, span<char const> fstr = {}) noexcept;
LIBIMP_EXPORT std::string to_string(signed long a, span<char const> fstr = {}) noexcept;
LIBIMP_EXPORT std::string to_string(unsigned long a, span<char const> fstr = {}) noexcept;
LIBIMP_EXPORT std::string to_string(signed long long a, span<char const> fstr = {}) noexcept;
LIBIMP_EXPORT std::string to_string(unsigned long long a, span<char const> fstr = {}) noexcept;
inline std::string to_string(  signed char a, span<char const> fstr = {}) noexcept { return to_string(     (int)a, fstr); }
inline std::string to_string(unsigned char a, span<char const> fstr = {}) noexcept { return to_string((unsigned)a, fstr); }

/// @brief Conversion of floating point type to strings.
/// @return an empty string if the conversion fails
LIBIMP_EXPORT std::string to_string(double a, span<char const> fstr = {}) noexcept;
LIBIMP_EXPORT std::string to_string(long double a, span<char const> fstr = {}) noexcept;
inline std::string to_string(float a, span<char const> fstr = {}) noexcept { return to_string((double)a, fstr); }

/// @brief Pointer.
inline std::string to_string(std::nullptr_t) noexcept { return "null"; }
template <typename T,
          typename = std::enable_if_t<std::is_same<T, void>::value>>
LIBIMP_EXPORT std::string to_string(T const volatile *a) noexcept;

/// @brief Date and time.
LIBIMP_EXPORT std::string to_string(std::tm const &a, span<char const> fstr = {}) noexcept;

namespace detail {

/**
 * @brief Convert std::time_t to std::string.
 * @return an empty string if the conversion fails
 */
inline std::string time_to_string(std::time_t tt, span<char const> fstr) noexcept {
#if defined(LIBIMP_CC_MSVC)
  /// @see https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/localtime-s-localtime32-s-localtime64-s
  std::tm tm {};
  if (::localtime_s(&tm, &tt) != 0) {
    return {};
  }
  return to_string(tm, fstr);
#else
  return to_string(*std::localtime(&tt), fstr);
#endif
}

} // namespace detail

template <class Clock, class Duration>
std::string to_string(std::chrono::time_point<Clock, Duration> const &a, span<char const> fstr = {}) noexcept {
  return detail::time_to_string(std::chrono::system_clock::to_time_t(a), fstr);
}

/**
 * @brief Predefined fmt_to_string method
 */
namespace detail {

template <typename T>
auto tag_invoke(fmt_to_string_t, T &&arg) noexcept
  -> decltype(::LIBIMP::to_string(std::forward<T>(arg))) {
  return ::LIBIMP::to_string(std::forward<T>(arg));
}

template <typename T>
auto tag_invoke(fmt_to_string_t, fmt_ref<T> arg) noexcept
  -> decltype(::LIBIMP::to_string(static_cast<T>(arg.param), arg.fstr)) {
  return ::LIBIMP::to_string(static_cast<T>(arg.param), arg.fstr);
}

template <typename T>
std::string tag_invoke(decltype(::LIBIMP::fmt_to_string), span<T> s) {
  if (s.empty()) {
    return {};
  }
  auto appender = fmt(s[0]);
  for (std::size_t i = 1; i < s.size(); ++i) {
    appender += fmt(' ', s[i]);
  }
  return appender;
}

} // namespace detail
LIBIMP_NAMESPACE_END_
