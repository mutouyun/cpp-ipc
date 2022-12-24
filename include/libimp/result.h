/**
 * \file libimp/result.h
 * \author mutouyun (orz@orzz.org)
 * \brief Define the return value type with a status code
 * \date 2022-04-17
 */
#pragma once

#include <type_traits>
#include <string>
#include <tuple>
#include <cstdint>

#include "libimp/def.h"
#include "libimp/detect_plat.h"
#include "libimp/export.h"
#include "libimp/fmt.h"
#include "libimp/generic.h"

LIBIMP_NAMESPACE_BEG_

using result_code_t = std::uint64_t;
using result_type   = std::tuple<result_code_t, bool>;

/**
 * \class class LIBIMP_EXPORT result_code
 * \brief Uses std::uint64_t as the default underlying type of error code.
 */
class LIBIMP_EXPORT result_code {
  result_type status_;

public:
  result_code() noexcept;
  result_code(result_code_t value) noexcept;
  result_code(result_type const &value) noexcept;
  result_code(bool ok, result_code_t value) noexcept;

  result_code_t value() const noexcept;
  bool ok() const noexcept;

  result_code_t operator*() const noexcept {
    return value();
  }
  explicit operator bool() const noexcept {
    return ok();
  }

  friend bool operator==(result_code const &lhs, result_code const &rhs) noexcept;
  friend bool operator!=(result_code const &lhs, result_code const &rhs) noexcept;
};

namespace detail_result {

template <typename T, typename = void>
struct default_traits;

} // namespace detail_result

template <typename T, 
          typename TypeTraits = detail_result::default_traits<T>>
class result;

namespace detail_result {

template <typename T>
struct default_traits<T, std::enable_if_t<std::is_integral<T>::value>> {
  /// \brief Custom initialization.
  constexpr static void init_code(result_code &code) noexcept {
    code = {};
  }
  constexpr static void init_code(result_code &code, bool ok, T value) noexcept {
    code = {ok, static_cast<result_code_t>(value)};
  }
  constexpr static void init_code(result_code &code, T value) noexcept {
    init_code(code, true, value);
  }

  /// \brief Custom default value.
  constexpr static T default_value() noexcept {
    return 0;
  }

  /// \brief Custom type conversions.
  constexpr static T cast_from_code(result_code code) noexcept {
    return static_cast<T>(code.value());
  }

  /// \brief Custom formatted output.
  static std::string format(result<T> const &r) noexcept {
    return fmt(*r);
  }
};

template <typename T>
struct default_traits<T, std::enable_if_t<std::is_pointer<T>::value>> {
  /// \brief Custom initialization.
  constexpr static void init_code(result_code &code, T value = default_value()) noexcept {
    code = {default_value() != value, reinterpret_cast<result_code_t>(value)};
  }
  constexpr static void init_code(result_code &code, std::nullptr_t) noexcept {
    code = {false, {}};
  }
  constexpr static void init_code(result_code &code, std::nullptr_t, result_code_t r) noexcept {
    code = {false, r};
  }

  /// \brief Custom default value.
  constexpr static T default_value() noexcept {
    return nullptr;
  }

  /// \brief Custom type conversions.
  constexpr static T cast_from_code(result_code code) noexcept {
    return code.ok() ? reinterpret_cast<T>(code.value()) : default_value();
  }

  /// \brief Custom formatted output.
  static std::string format(result<T> const &r) noexcept {
    if LIBIMP_LIKELY(r) {
      return fmt(static_cast<void *>(*r));
    }
    return fmt(static_cast<void *>(*r), ", code = ", r.code_value());
  }
};

} // namespace detail_result

/**
 * \class class result
 * \brief The generic wrapper for the result_code.
 */
template <typename T, typename TypeTraits>
class result : public TypeTraits {

  /// \brief Internal data is stored using result_code.
  result_code code_;

public:
  using type_traits_t = TypeTraits;

  template <typename... A, 
            typename = is_not_match<result, A...>,
            typename = decltype(type_traits_t::init_code(std::declval<result_code &>()
                                                       , std::declval<A>()...))>
  result(A &&... args) noexcept {
    type_traits_t::init_code(code_, std::forward<A>(args)...);
  }

  bool ok() const noexcept { return code_.ok(); }
  T value() const noexcept { return type_traits_t::cast_from_code(code_); }

  result_code_t code_value() const noexcept { return code_.value(); }

  T operator*() const noexcept {
    return value();
  }
  explicit operator bool() const noexcept {
    return ok();
  }

  friend bool operator==(result const &lhs, result const &rhs) noexcept { return lhs.code_ == rhs.code_; }
  friend bool operator!=(result const &lhs, result const &rhs) noexcept { return lhs.code_ != rhs.code_; }
};

/// \brief Custom defined fmt_to method for imp::fmt
namespace detail {

inline bool tag_invoke(decltype(::LIBIMP::fmt_to), fmt_context &ctx, result_code r) {
  return fmt_to(ctx, "[", (r ? "succ" : "fail"), ", value = ", *r, "]");
}

template <typename T, typename D>
bool tag_invoke(decltype(::LIBIMP::fmt_to), fmt_context &ctx, result<T, D> r) {
  return fmt_to(ctx, "[", (r ? "succ" : "fail"), ", value = ", result<T, D>::type_traits_t::format(r), "]");
}

} // namespace detail
LIBIMP_NAMESPACE_END_
