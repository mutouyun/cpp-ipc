/**
 * @file libimp/result.h
 * @author mutouyun (orz@orzz.org)
 * @brief Define the return value type with a status code
 * @date 2022-04-17
 */
#pragma once

#include <type_traits>
#include <cstdint>

#include "fmt/format.h"

#include "libimp/def.h"
#include "libimp/detect_plat.h"
#include "libimp/export.h"

LIBIMP_NAMESPACE_BEG_

using result_type = std::uint64_t;

/**
 * @brief Use the least significant (in Little-Endian) of 
 * a 64-bit unsigned integer to indicate success or failure,
 * so the data significant bit cannot exceed 63 bits.
 */
class LIBIMP_EXPORT result_code {
  result_type status_;

public:
  result_code() noexcept;
  result_code(result_type value) noexcept;
  result_code(bool ok, result_type value) noexcept;

  result_type value() const noexcept;
  bool ok() const noexcept;

  result_type operator*() const noexcept {
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
          typename DefTraits = detail_result::default_traits<T>, 
          typename = void>
class result;

namespace detail_result {

template <typename T>
struct default_traits<T, std::enable_if_t<std::is_integral<T>::value>> {
  constexpr static T value() noexcept { return 0; }

  static result_type cast_to_code(T value) noexcept {
    return static_cast<result_type>(value);
  }
  static T cast_from_code(result_type code) noexcept {
    return static_cast<T>(code);
  }
  template <typename Out>
  static auto format(result<T> const &r, Out &&out) noexcept {
    return format_to(out, "{}", *r);
  }
};

template <typename T>
struct default_traits<T, std::enable_if_t<std::is_pointer<T>::value>> {
  constexpr static T value() noexcept { return nullptr; }

  static result_type cast_to_code(T value) noexcept {
    return reinterpret_cast<result_type>(value);
  }
  static T cast_from_code(result_type code) noexcept {
    return reinterpret_cast<T>(code);
  }
  template <typename Out>
  static auto format(result<T> const &r, Out &&out) noexcept {
    if LIBIMP_LIKELY(r) {
      return format_to(out, "{}", static_cast<void *>(*r));
    }
    return format_to(out, "{}, code = {}", static_cast<void *>(*r), r.code_value());
  }
};

} // namespace detail_result

template <typename T, typename DefTraits>
class result<T, DefTraits, std::enable_if_t<std::is_integral<T>::value>> {

  result_code code_;

public:
  using default_traits_t = DefTraits;

  result() noexcept = default;

  result(T value) noexcept
    : result(true, std::move(value)) {}

  result(bool ok, T value) noexcept
    : code_(ok, default_traits_t::cast_to_code(value)) {}

  T value() const noexcept { return default_traits_t::cast_from_code(code_.value()); }
  bool ok() const noexcept { return code_.ok(); }

  T operator*() const noexcept {
    return value();
  }
  explicit operator bool() const noexcept {
    return ok();
  }

  friend bool operator==(result const &lhs, result const &rhs) noexcept { return lhs.code_ == rhs.code_; }
  friend bool operator!=(result const &lhs, result const &rhs) noexcept { return lhs.code_ != rhs.code_; }
};

template <typename T, typename DefTraits>
class result<T, DefTraits, std::enable_if_t<std::is_pointer<T>::value>> {

  result_code code_;

public:
  using default_traits_t = DefTraits;

  result(T value = default_traits_t::value()) noexcept
    : code_(default_traits_t::value() != value, 
            default_traits_t::cast_to_code(value)) {}

  result(std::nullptr_t, result_type code) noexcept
    : code_(false, code) {}

  T value() const noexcept { return code_.ok() ? default_traits_t::cast_from_code(code_.value()) : nullptr; }
  bool ok() const noexcept { return code_.ok(); }
  result_type code_value() const noexcept { return code_.value(); }

  T operator*() const noexcept {
    return value();
  }
  explicit operator bool() const noexcept {
    return ok();
  }

  friend bool operator==(result const &lhs, result const &rhs) noexcept { return lhs.code_ == rhs.code_; }
  friend bool operator!=(result const &lhs, result const &rhs) noexcept { return lhs.code_ != rhs.code_; }
};

LIBIMP_NAMESPACE_END_

template <typename T, typename D>
struct fmt::formatter<::LIBIMP::result<T, D>> {
  constexpr auto parse(format_parse_context& ctx) const {
    return ctx.end();
  }
  template <typename FormatContext>
  auto format(::LIBIMP::result<T, D> r, FormatContext &ctx) {
    return format_to(::LIBIMP::result<T, D>::default_traits_t::format(r, 
           format_to(ctx.out(), "[{}, value = ", r ? "succ" : "fail")), "]");
  }
};

template <>
struct fmt::formatter<::LIBIMP::result_code>
          : formatter<::LIBIMP::result<::LIBIMP::result_type>> {
  template <typename FormatContext>
  auto format(::LIBIMP::result_code r, FormatContext &ctx) {
    return format_to(ctx.out(), "[{}, value = {}]", (r ? "succ" : "fail"), *r);
  }
};
