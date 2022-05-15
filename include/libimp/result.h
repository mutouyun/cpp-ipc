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
#include "libimp/export.h"

LIBIMP_NAMESPACE_BEG_

/**
 * @brief Use the least significant (in Little-Endian) of 
 * a 64-bit unsigned integer to indicate success or failure,
 * so the data significant bit cannot exceed 63 bits.
 */
class LIBIMP_EXPORT result_code {
  std::uint64_t status_;

public:
  result_code() noexcept;
  result_code(bool ok, std::uint64_t value = 0) noexcept;

  std::uint64_t value() const noexcept;
  bool ok() const noexcept;

  std::uint64_t operator*() const noexcept {
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

template <typename T>
struct default_traits<T, std::enable_if_t<std::is_integral<T>::value>> {
  constexpr static T value() noexcept { return 0; }

  static std::uint64_t cast_to_code(T value) noexcept {
    return static_cast<std::uint64_t>(value);
  }
  static T cast_from_code(std::uint64_t code) noexcept {
    return static_cast<T>(code);
  }
  static T regularize(T value) noexcept {
    return value;
  }
};

template <typename T>
struct default_traits<T, std::enable_if_t<std::is_pointer<T>::value>> {
  constexpr static T value() noexcept { return nullptr; }

  static std::uint64_t cast_to_code(T value) noexcept {
    return reinterpret_cast<std::uint64_t>(value);
  }
  static T cast_from_code(std::uint64_t code) noexcept {
    return reinterpret_cast<T>(code);
  }
  static void *regularize(T value) noexcept {
    return value;
  }
};

} // namespace detail_result

template <typename T, 
          typename DefTraits = detail_result::default_traits<T>>
class result {

  result_code code_;

public:
  using default_traits_t = DefTraits;

  result() noexcept = default;
  result(bool ok, T value = default_traits_t::value())
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

  friend std::ostream &operator<<(std::ostream &o, result const &r) noexcept { return o << r.code_; }
};

LIBIMP_NAMESPACE_END_

template <typename T>
struct fmt::formatter<::LIBIMP_NAMESPACE::result<T>> {
  constexpr auto parse(format_parse_context& ctx) {
    return ctx.end();
  }

  template <typename FormatContext>
  auto format(::LIBIMP_NAMESPACE::result<T> r, FormatContext &ctx) {
    return format_to(ctx.out(), "[{}, value = {}]", 
                    (r ? "succ" : "fail"), 
                    ::LIBIMP_NAMESPACE::result<T>::default_traits_t::regularize(*r));
  }
};

template <>
struct fmt::formatter<::LIBIMP_NAMESPACE::result_code>
          : formatter<::LIBIMP_NAMESPACE::result<std::uint64_t>> {
  template <typename FormatContext>
  auto format(::LIBIMP_NAMESPACE::result_code r, FormatContext &ctx) {
    return format_to(ctx.out(), "[{}, value = {}]", (r ? "succ" : "fail"), *r);
  }
};
