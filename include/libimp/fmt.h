/**
 * @file libimp/fmt.h
 * @author mutouyun (orz@orzz.org)
 * @brief String formatting.
 * @date 2022-11-26
 */
#pragma once

#include <string>
#include <utility>
#include <type_traits>
#include <chrono>   // std::chrono::time_point
#include <cstddef>
#include <ctime>    // std::tm

#include "libimp/def.h"
#include "libimp/fmt_cpo.h"
#include "libimp/span.h"
#include "libimp/detect_plat.h"
#include "libimp/export.h"

LIBIMP_NAMESPACE_BEG_

template <typename T>
struct fmt_ref {
  span<char const> fstr;
  T param;
};

template <std::size_t N>
auto spec(char const (&fstr)[N]) noexcept {
  return [&fstr](auto &&arg) noexcept {
    using arg_t = decltype(arg);
    return fmt_ref<arg_t> {{fstr}, static_cast<arg_t>(arg)};
  };
}

template <typename... A>
std::string fmt(A &&...args) {
  std::string joined;
  LIBIMP_UNUSED auto unfold = {
    joined.append(fmt_to_string(std::forward<A>(args)))...
  };
  return joined;
}

/// @brief Return the string directly.
LIBIMP_EXPORT std::string to_string(std::string const &a) noexcept;
LIBIMP_EXPORT std::string to_string(std::string &&a) noexcept;
LIBIMP_EXPORT std::string to_string(std::string const &a, span<char const> fstr) noexcept;

/// @brief Character to string conversion.
/// @return an empty string if the conversion fails
LIBIMP_EXPORT std::string to_string(char a) noexcept;
LIBIMP_EXPORT std::string to_string(wchar_t a) noexcept;
LIBIMP_EXPORT std::string to_string(char16_t a) noexcept;
LIBIMP_EXPORT std::string to_string(char32_t a) noexcept;
#if defined(LIBIMP_CPP_20)
LIBIMP_EXPORT std::string to_string(char8_t a) noexcept;
#endif // defined(LIBIMP_CPP_20)

/// @brief Conversion of numeric types to strings.
/// @return an empty string if the conversion fails
LIBIMP_EXPORT std::string to_string(signed char a, span<char const> fstr = {}) noexcept;
LIBIMP_EXPORT std::string to_string(unsigned char a, span<char const> fstr = {}) noexcept;
LIBIMP_EXPORT std::string to_string(signed short a, span<char const> fstr = {}) noexcept;
LIBIMP_EXPORT std::string to_string(unsigned short a, span<char const> fstr = {}) noexcept;
LIBIMP_EXPORT std::string to_string(signed int a, span<char const> fstr = {}) noexcept;
LIBIMP_EXPORT std::string to_string(unsigned int a, span<char const> fstr = {}) noexcept;
LIBIMP_EXPORT std::string to_string(signed long a, span<char const> fstr = {}) noexcept;
LIBIMP_EXPORT std::string to_string(unsigned long a, span<char const> fstr = {}) noexcept;
LIBIMP_EXPORT std::string to_string(signed long long a, span<char const> fstr = {}) noexcept;
LIBIMP_EXPORT std::string to_string(unsigned long long a, span<char const> fstr = {}) noexcept;

/// @brief Conversion of floating point type to strings.
/// @return an empty string if the conversion fails
LIBIMP_EXPORT std::string to_string(float a, span<char const> fstr = {}) noexcept;
LIBIMP_EXPORT std::string to_string(double a, span<char const> fstr = {}) noexcept;
LIBIMP_EXPORT std::string to_string(long double a, span<char const> fstr = {}) noexcept;

/// @brief Pointer.
LIBIMP_EXPORT std::string to_string(std::nullptr_t) noexcept;
template <typename T,
          typename = std::enable_if_t<std::is_same<T, void>::value>>
LIBIMP_EXPORT std::string to_string(T const volatile *a) noexcept;

/// @brief Date and time.
LIBIMP_EXPORT std::string to_string(std::tm const &a, span<char const> fstr = {}) noexcept;
namespace detail {
LIBIMP_EXPORT std::string time_to_string(std::time_t tt, span<char const> fstr) noexcept;
} // namespace detail
template <class Clock, class Duration>
LIBIMP_EXPORT std::string to_string(std::chrono::time_point<Clock, Duration> const &a, span<char const> fstr = {}) noexcept {
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

} // namespace detail
LIBIMP_NAMESPACE_END_
