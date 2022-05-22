/**
 * @file libimp/log.h
 * @author mutouyun (orz@orzz.org)
 * @brief Simple log output component
 * @date 2022-05-22
 */
#pragma once

#include <string>
#include <type_traits>

#include "fmt/format.h"

#include "libimp/def.h"
#include "libimp/detect_plat.h"
#include "libimp/export.h"

LIBIMP_NAMESPACE_BEG_

namespace detail_log {

template <typename T>
class has_fn_info {
  static std::false_type check(...);
  template <typename U>
  static auto check(U *u) -> decltype(u->info(std::declval<std::string>()), std::true_type{});

public:
  using type = decltype(check(static_cast<T *>(nullptr)));
};

template <typename T>
constexpr bool has_fn_info_v = has_fn_info<T>::type::value;

template <typename T>
class has_fn_error {
  static std::false_type check(...);
  template <typename U>
  static auto check(U *u) -> decltype(u->error(std::declval<std::string>()), std::true_type{});

public:
  using type = decltype(check(static_cast<T *>(nullptr)));
};

template <typename T>
constexpr bool has_fn_error_v = has_fn_error<T>::type::value;

struct vtable_t {
  void (*info )(void *, std::string &&);
  void (*error)(void *, std::string &&);
};

template <typename T>
class traits {
  template <typename U>
  static auto make_fn_info() noexcept
    -> std::enable_if_t<has_fn_info_v<U>, void (*)(void *, std::string &&)> {
    return [](void *p, std::string &&s) {
      static_cast<U *>(p)->info(std::move(s));
    };
  }
  template <typename U>
  static auto make_fn_info() noexcept
    -> std::enable_if_t<!has_fn_info_v<U>, void (*)(void *, std::string &&)> {
    return [](void *, std::string &&) {};
  }

  template <typename U>
  static auto make_fn_error() noexcept
    -> std::enable_if_t<has_fn_error_v<U>, void (*)(void *, std::string &&)> {
    return [](void *p, std::string &&s) {
      static_cast<U *>(p)->error(std::move(s));
    };
  }
  template <typename U>
  static auto make_fn_error() noexcept
    -> std::enable_if_t<!has_fn_error_v<U>, void (*)(void *, std::string &&)> {
    return [](void *, std::string &&) {};
  }

public:
  static auto make_vtable() noexcept {
    static vtable_t vt {
      make_fn_info <T>(),
      make_fn_error<T>(),
    };
    return &vt;
  }
};

class prefix {
  std::string px_;

public:
  template <typename... A>
  prefix(A &&... args) {
    LIBIMP_UNUSED auto unfold = {
      0, ((px_ += ::fmt::format("[{}]", std::forward<A>(args))), 0)...
    };
  }

  operator std::string() const noexcept {
    return px_;
  }
};

} // namespace detail_log

class LIBIMP_EXPORT log_printer {
  void *objp_ {nullptr};
  detail_log::vtable_t *vtable_ {nullptr};

public:
  log_printer() noexcept = default;

  template <typename T>
  log_printer(T &p) noexcept
    : objp_  (static_cast<void *>(&p))
    , vtable_(detail_log::traits<T>::make_vtable()) {}

  explicit operator bool() const noexcept;

  void info (std::string &&);
  void error(std::string &&);
};

class LIBIMP_EXPORT log_std_t {
public:
  void info (std::string &&) const;
  void error(std::string &&) const;
};

LIBIMP_EXPORT extern log_std_t log_std;

namespace log {

template <typename Fmt, typename... A>
std::string fmt(Fmt &&ft, A &&... args) {
  return ::fmt::format(std::forward<Fmt>(ft), std::forward<A>(args)...);
}

} // namespace log

LIBIMP_NAMESPACE_END_
