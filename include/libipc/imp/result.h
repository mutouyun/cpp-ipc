/**
 * \file libipc/result.h
 * \author mutouyun (orz@orzz.org)
 * \brief Define the return value type with an error status code.
 */
#pragma once

#include <type_traits>
#include <utility>
#include <cstdint>

#include "libipc/imp/expected.h"
#include "libipc/imp/error.h"
#include "libipc/imp/generic.h"
#include "libipc/imp/fmt.h"

namespace ipc {
namespace detail_result {

template <typename T>
struct generic_initializer {
  using storage_t = expected<T, std::error_code>;
  
  /// \brief Custom initialization.
  static constexpr storage_t init_code() noexcept {
    return {unexpected, std::error_code(-1, std::generic_category())};
  }

  static constexpr storage_t init_code(T value) noexcept {
    return {in_place, value};
  }

  static constexpr storage_t init_code(std::error_code const &ec) noexcept {
    return {unexpected, ec};
  }
};

template <typename T, typename = void>
struct result_base;

template <typename T>
struct result_base<T, std::enable_if_t<std::is_pointer<T>::value>>
  : generic_initializer<T> {

  using storage_t = typename generic_initializer<T>::storage_t;
  using generic_initializer<T>::init_code;

  static constexpr storage_t init_code(std::nullptr_t, std::error_code const &ec) noexcept {
    return {unexpected, ec};
  }

  static constexpr storage_t init_code(std::nullptr_t) noexcept {
    return {unexpected, std::error_code(-1, std::generic_category())};
  }
};

template <typename T>
struct result_base<T, std::enable_if_t<std::is_integral<T>::value || std::is_enum<T>::value>>
  : generic_initializer<T> {

  using storage_t = typename generic_initializer<T>::storage_t;
  using generic_initializer<T>::init_code;
};

} // namespace detail_result

/**
 * \class class result
 * \brief The generic wrapper for the result type.
 */
template <typename T>
class result : public detail_result::result_base<T> {
private:
  using base_t    = detail_result::result_base<T>;
  using storage_t = typename base_t::storage_t;

  storage_t ret_; ///< internal data

public:
  template <typename... A, 
            typename = not_match<result, A...>,
            typename = decltype(base_t::init_code(std::declval<A>()...))>
  result(A &&...args) noexcept
    : ret_(base_t::init_code(std::forward<A>(args)...)) {}

  std::string format_string() const {
    if LIBIPC_LIKELY(ret_) {
      return fmt("value = ", ret_.value());
    } else {
      return fmt("error = ", ret_.error());
    }
  }

  T               value() const noexcept { return ret_ ? ret_.value() : T{}; }
  bool            ok   () const noexcept { return ret_.has_value(); }
  std::error_code error() const noexcept { return ret_.error(); }

         T operator *   () const noexcept { return value(); }
  explicit operator bool() const noexcept { return ok   (); }

  friend bool operator==(result const &lhs, result const &rhs) noexcept { return lhs.ret_ == rhs.ret_; }
  friend bool operator!=(result const &lhs, result const &rhs) noexcept { return !(lhs == rhs); }
};

template <>
class result<void> {
private:
  std::error_code ret_; ///< internal data

public:
  result() noexcept
    : ret_(-1, std::generic_category()) {}
  
  result(std::error_code const &ec) noexcept
    : ret_(ec) {}

  std::string format_string() const {
    return fmt("error = ", error());
  }

  bool              ok   () const noexcept { return !ret_; }
  std::error_code   error() const noexcept { return  ret_; }
  explicit operator bool () const noexcept { return  ok(); }

  friend bool operator==(result const &lhs, result const &rhs) noexcept { return lhs.ret_ == rhs.ret_; }
  friend bool operator!=(result const &lhs, result const &rhs) noexcept { return !(lhs == rhs); }
};

/// \brief Custom defined fmt_to method for imp::fmt
namespace detail_tag_invoke {

template <typename T>
inline bool tag_invoke(decltype(ipc::fmt_to), fmt_context &ctx, result<T> const &r) {
  return fmt_to(ctx, (r ? "succ" : "fail"), ", ", r.format_string());
}

} // namespace detail_tag_invoke
} // namespace ipc
