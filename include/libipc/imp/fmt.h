/**
 * \file libipc/fmt.h
 * \author mutouyun (orz@orzz.org)
 * \brief String formatting.
 * 
 * \remarks The current performance is not high, 
 * because I use std::sprintf directly for formatting for convenience.
 */
#pragma once

#include <string>
#include <utility>
#include <type_traits>
#include <chrono>   // std::chrono::time_point
#include <tuple>
#include <cstddef>
#include <ctime>    // std::tm, std::localtime

#include "libipc/imp/fmt_cpo.h"
#include "libipc/imp/span.h"
#include "libipc/imp/detect_plat.h"
#include "libipc/imp/export.h"

namespace ipc {

/**
 * \brief The format string reference wrapper.
 */
template <typename T>
struct fmt_ref {
  span<char const> fstr;
  T param;
};

/**
 * \brief Conversion specifiers.
 * 
 * \remarks Just like printf, the format string is of the form
 * [flags][field_width][.precision][conversion_character]
 * 
 * \see http://personal.ee.surrey.ac.uk/Personal/R.Bowden/C/printf.html
 */
template <std::size_t N>
auto spec(char const (&fstr)[N]) noexcept {
  return [&fstr](auto &&arg) noexcept {
    using arg_t = decltype(arg);
    return fmt_ref<arg_t> {{fstr}, static_cast<arg_t>(arg)};
  };
}

/**
 * \brief String formatting function.
 * 
 * \param args arguments that support the fmt output
 * \return an empty string if the fmt output fails 
 */
template <typename... A>
LIBIPC_NODISCARD std::string fmt(A &&...args) {
  std::string joined;
  fmt_context ctx(joined);
  if (fmt_to(ctx, std::forward<A>(args)...)) {
    return ctx.finish() ? joined : "";
  }
  return {};
}

/// \brief String types.
LIBIPC_EXPORT bool to_string(fmt_context &ctx, char const *       a) noexcept;
LIBIPC_EXPORT bool to_string(fmt_context &ctx, std::string const &a) noexcept;
LIBIPC_EXPORT bool to_string(fmt_context &ctx, char const *       a, span<char const> fstr) noexcept;
       inline bool to_string(fmt_context &ctx, std::string const &a, span<char const> fstr) noexcept { return to_string(ctx, a.c_str(), fstr); }

/// \brief Character to string conversion.
LIBIPC_EXPORT bool to_string(fmt_context &ctx, char     a) noexcept;
#if defined(LIBIPC_CPP_20)
       inline bool to_string(fmt_context &ctx, char8_t  a) noexcept { return to_string(ctx, (char)a); }
#endif // defined(LIBIPC_CPP_20)
LIBIPC_EXPORT bool to_string(fmt_context &ctx, wchar_t  a) noexcept;
LIBIPC_EXPORT bool to_string(fmt_context &ctx, char16_t a) noexcept;
LIBIPC_EXPORT bool to_string(fmt_context &ctx, char32_t a) noexcept;

/// \brief Conversion of numeric types to strings.
LIBIPC_EXPORT bool to_string(fmt_context &ctx,   signed short     a, span<char const> fstr = {}) noexcept;
LIBIPC_EXPORT bool to_string(fmt_context &ctx, unsigned short     a, span<char const> fstr = {}) noexcept;
LIBIPC_EXPORT bool to_string(fmt_context &ctx,   signed int       a, span<char const> fstr = {}) noexcept;
LIBIPC_EXPORT bool to_string(fmt_context &ctx, unsigned int       a, span<char const> fstr = {}) noexcept;
LIBIPC_EXPORT bool to_string(fmt_context &ctx,   signed long      a, span<char const> fstr = {}) noexcept;
LIBIPC_EXPORT bool to_string(fmt_context &ctx, unsigned long      a, span<char const> fstr = {}) noexcept;
LIBIPC_EXPORT bool to_string(fmt_context &ctx,   signed long long a, span<char const> fstr = {}) noexcept;
LIBIPC_EXPORT bool to_string(fmt_context &ctx, unsigned long long a, span<char const> fstr = {}) noexcept;
       inline bool to_string(fmt_context &ctx,   signed char      a, span<char const> fstr = {}) noexcept { return to_string(ctx,      (int)a, fstr); }
       inline bool to_string(fmt_context &ctx, unsigned char      a, span<char const> fstr = {}) noexcept { return to_string(ctx, (unsigned)a, fstr); }

/// \brief Conversion of floating point type to strings.
LIBIPC_EXPORT bool to_string(fmt_context &ctx,      double a, span<char const> fstr = {}) noexcept;
LIBIPC_EXPORT bool to_string(fmt_context &ctx, long double a, span<char const> fstr = {}) noexcept;
       inline bool to_string(fmt_context &ctx, float       a, span<char const> fstr = {}) noexcept { return to_string(ctx, (double)a, fstr); }

/// \brief Pointer.
LIBIPC_EXPORT bool to_string(fmt_context &ctx, std::nullptr_t) noexcept;
template <typename T,
          typename = std::enable_if_t<std::is_same<T, void>::value>>
LIBIPC_EXPORT bool to_string(fmt_context &ctx, T const volatile *a) noexcept;

/// \brief Date and time.
LIBIPC_EXPORT bool to_string(fmt_context &ctx, std::tm const &a, span<char const> fstr = {}) noexcept;

namespace detail_fmt {

/**
 * \brief Convert std::time_t to std::string.
 * \return an empty string if the conversion fails
 */
inline bool time_to_string(fmt_context &ctx, std::time_t tt, span<char const> fstr) noexcept {
#if defined(LIBIPC_CC_MSVC)
  /// \see https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/localtime-s-localtime32-s-localtime64-s
  std::tm tm {};
  if (::localtime_s(&tm, &tt) != 0) {
    return {};
  }
  return to_string(ctx, tm, fstr);
#else
  return to_string(ctx, *std::localtime(&tt), fstr);
#endif
}

} // namespace detail_fmt

template <class Clock, class Duration>
bool to_string(fmt_context &ctx, std::chrono::time_point<Clock, Duration> const &a, span<char const> fstr = {}) noexcept {
  return detail_fmt::time_to_string(ctx, std::chrono::system_clock::to_time_t(a), fstr);
}

/**
 * \brief Predefined fmt_to method
 */
namespace detail_tag_invoke {

template <typename T>
auto tag_invoke(decltype(ipc::fmt_to), fmt_context &ctx, T &&arg) noexcept
  -> decltype(ipc::to_string(ctx, std::forward<T>(arg))) {
  return ipc::to_string(ctx, std::forward<T>(arg));
}

template <typename T>
auto tag_invoke(decltype(ipc::fmt_to), fmt_context &ctx, fmt_ref<T> arg) noexcept
  -> decltype(ipc::to_string(ctx, static_cast<T>(arg.param), arg.fstr)) {
  return ipc::to_string(ctx, static_cast<T>(arg.param), arg.fstr);
}

template <typename T>
bool tag_invoke(decltype(ipc::fmt_to), fmt_context &ctx, span<T> s) {
  if (s.empty()) {
    return false;
  }
  if (!fmt_to(ctx, s[0])) {
    return false;
  }
  for (std::size_t i = 1; i < s.size(); ++i) {
    if (!fmt_to(ctx, ' ', s[i])) return false;
  }
  return true;
}

template <typename Tp, std::size_t... I>
bool unfold_tuple_fmt_to(fmt_context &ctx, Tp const &tp, std::index_sequence<I...>) {
  return fmt_to(ctx, std::get<I>(tp)...);
}

template <typename... T>
bool tag_invoke(decltype(ipc::fmt_to), fmt_context &ctx, std::tuple<T...> const &tp) {
  return unfold_tuple_fmt_to(ctx, tp, std::index_sequence_for<T...>{});
}

} // namespace detail_tag_invoke
} // namespace ipc
