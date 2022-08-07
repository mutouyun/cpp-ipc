/**
 * @file libimp/log.h
 * @author mutouyun (orz@orzz.org)
 * @brief Simple log output component
 * @date 2022-05-22
 */
#pragma once

#include <string>
#include <type_traits>
#include <chrono>
#include <exception>

#include "fmt/format.h"
#include "fmt/chrono.h"

#include "libimp/def.h"
#include "libimp/detect_plat.h"
#include "libimp/export.h"
#include "libimp/enum_cast.h"

LIBIMP_NAMESPACE_BEG_
namespace log {

template <typename Fmt, typename... A>
std::string fmt(Fmt &&ft, A &&... args) {
  return ::fmt::format(std::forward<Fmt>(ft), std::forward<A>(args)...);
}

enum class level : std::int32_t {
  trace,
  debug,
  info,
  warning,
  error,
  failed,
};

} // namespace log

namespace detail_log {

template <typename T>
class has_fn_output {
  static std::false_type check(...);
  template <typename U>
  static auto check(U *u) -> decltype(u->output(log::level::trace, std::declval<std::string>()), std::true_type{});

public:
  using type = decltype(check(static_cast<T *>(nullptr)));
};

template <typename T>
constexpr bool has_fn_output_v = has_fn_output<T>::type::value;

struct vtable_t {
  void (*output)(void *, log::level, std::string &&) noexcept;
};

template <typename T>
class traits {
  template <typename U>
  static auto make_fn_output() noexcept
    -> std::enable_if_t<has_fn_output_v<U>, void (*)(void *, log::level, std::string &&) noexcept> {
    return [](void *p, log::level l, std::string &&s) noexcept {
      static_cast<U *>(p)->output(l, std::move(s));
    };
  }
  template <typename U>
  static auto make_fn_output() noexcept
    -> std::enable_if_t<!has_fn_output_v<U>, void (*)(void *, log::level, std::string &&) noexcept> {
    return [](void *, log::level, std::string &&) noexcept {};
  }

public:
  static auto make_vtable() noexcept {
    static vtable_t vt {
      make_fn_output<T>(),
    };
    return &vt;
  }
};

} // namespace detail_log

namespace log {

class LIBIMP_EXPORT printer {
  void *objp_ {nullptr};
  detail_log::vtable_t *vtable_ {nullptr};

public:
  printer() noexcept = default;

  template <typename T>
  printer(T &p) noexcept
    : objp_  (static_cast<void *>(&p))
    , vtable_(detail_log::traits<T>::make_vtable()) {}

  explicit operator bool() const noexcept;

  void output(log::level, std::string &&) noexcept;
};

class LIBIMP_EXPORT std_t {
public:
  void output(log::level, std::string &&) noexcept;
};

LIBIMP_EXPORT extern std_t std_out;

class gripper {
  printer printer_;
  char const *func_;
  level level_limit_;

  template <typename Fmt, typename... A>
  gripper &output(log::level l, Fmt &&ft, A &&... args) noexcept {
    if (!printer_ || (enum_cast(l) < enum_cast(level_limit_))) {
      return *this;
    }
    constexpr static char types[] = {
      'T', 'D', 'I', 'W', 'E', 'F'
    };
    try {
      auto tp = std::chrono::system_clock::now();
      auto ms = std::chrono::time_point_cast<std::chrono::milliseconds>(tp).time_since_epoch().count() % 1000;
      auto px = fmt("[{}][{:%Y-%m-%d %H:%M:%S}.{:03}][{}] ", types[enum_cast(l)], tp, ms, func_);
      printer_.output(l, std::move(px += fmt(std::forward<Fmt>(ft), std::forward<A>(args)...)));
    } catch (std::exception const &e) {
      /// @brief [TBD] std::string constructor may throw an exception
      printer_.output(level::failed, e.what());
    }
    return *this;
  }

public:
  gripper(char const *func, printer printer = std_out, level level_limit = level::info) noexcept 
    : printer_    (printer)
    , func_       (func)
    , level_limit_(level_limit) {}

  template <typename Fmt, typename... A>
  gripper &trace(Fmt &&ft, A &&... args) noexcept {
    return output(log::level::trace, std::forward<Fmt>(ft), std::forward<A>(args)...);
  }
  template <typename Fmt, typename... A>
  gripper &debug(Fmt &&ft, A &&... args) noexcept {
    return output(log::level::debug, std::forward<Fmt>(ft), std::forward<A>(args)...);
  }
  template <typename Fmt, typename... A>
  gripper &info(Fmt &&ft, A &&... args) noexcept {
    return output(log::level::info, std::forward<Fmt>(ft), std::forward<A>(args)...);
  }
  template <typename Fmt, typename... A>
  gripper &warning(Fmt &&ft, A &&... args) noexcept {
    return output(log::level::warning, std::forward<Fmt>(ft), std::forward<A>(args)...);
  }
  template <typename Fmt, typename... A>
  gripper &error(Fmt &&ft, A &&... args) noexcept {
    return output(log::level::error, std::forward<Fmt>(ft), std::forward<A>(args)...);
  }
  template <typename Fmt, typename... A>
  gripper &failed(Fmt &&ft, A &&... args) noexcept {
    return output(log::level::failed, std::forward<Fmt>(ft), std::forward<A>(args)...);
  }
};

} // namespace log
LIBIMP_NAMESPACE_END_

#define LIBIMP_LOG_(...) LIBIMP_NAMESPACE_::log::gripper log {__func__, __VA_ARGS__}