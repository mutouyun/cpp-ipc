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

#include "libimp/def.h"
#include "libimp/detect_plat.h"
#include "libimp/export.h"
#include "libimp/enum_cast.h"

LIBIMP_NAMESPACE_BEG_
namespace log {

enum class level : std::int32_t {
  trace,
  debug,
  info,
  warning,
  error,
  failed,
};

struct context {
  log::level level;
  std::chrono::system_clock::time_point tp;
  char const *func;
  std::string text;
};

LIBIMP_EXPORT std::string to_string(context &&) noexcept;

} // namespace log

namespace detail_log {

enum out_type : unsigned {
  out_none    = 0x0,
  out_string  = 0x1,
  out_context = 0x2,
};

template <typename T>
class has_fn_output {
  static std::integral_constant<out_type, out_none> check(...);

  template <typename U, typename = decltype(std::declval<U &>().output(log::level::trace, std::declval<std::string>()))>
  static std::integral_constant<out_type, out_string> check(U *u);

  template <typename U, typename = decltype(std::declval<U &>().output(std::declval<log::context>()))>
  static std::integral_constant<out_type, out_context> check(U *u);

public:
  using type = decltype(check(static_cast<T *>(nullptr)));
};

template <typename T>
constexpr out_type has_fn_output_v = has_fn_output<T>::type::value;

struct vtable_t {
  void (*output)(void *, log::context &&);
};

template <typename T>
class traits {
  template <typename U>
  static auto make_fn_output() noexcept
    -> std::enable_if_t<(has_fn_output_v<U> == out_none), decltype(vtable_t{}.output)> {
    return [](void *, log::context &&) {};
  }

  template <typename U>
  static auto make_fn_output() noexcept
    -> std::enable_if_t<(has_fn_output_v<U> == out_string), decltype(vtable_t{}.output)> {
    return [](void *p, log::context &&ctx) {
      auto lev = ctx.level;
      auto str = to_string(std::move(ctx));
      static_cast<U *>(p)->output(lev, std::move(str));
    };
  }

  template <typename U>
  static auto make_fn_output() noexcept
    -> std::enable_if_t<(has_fn_output_v<U> == out_context), decltype(vtable_t{}.output)> {
    return [](void *p, log::context &&ctx) {
      static_cast<U *>(p)->output(std::move(ctx));
    };
  }

public:
  static auto make_vtable() noexcept {
    static vtable_t vt { make_fn_output<T>() };
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

  template <typename T, 
            /// @remark generic constructor may shadow the default copy constructor
            typename = std::enable_if_t<!std::is_same<printer, T>::value>>
  printer(T &p) noexcept
    : objp_  (static_cast<void *>(&p))
    , vtable_(detail_log::traits<T>::make_vtable()) {}

  explicit operator bool() const noexcept;
  void output(context) noexcept;
};

/// @brief Standard console output.
class LIBIMP_EXPORT std_t {
public:
  void output(log::level, std::string &&) noexcept;
};

/// @brief Standard console output object.
LIBIMP_EXPORT extern std_t std_out;

/**
 * @brief Log information grips.
 */
class grip {
  printer printer_;
  char const *func_;
  level level_limit_;

  template <typename Fmt, typename... A>
  grip &output(log::level l, Fmt &&ft, A &&... args) noexcept {
    if (!printer_ || (enum_cast(l) < enum_cast(level_limit_))) {
      return *this;
    }
    context ctx;
    LIBIMP_TRY {
      ctx = {
        l, std::chrono::system_clock::now(), func_,
        ::fmt::format(std::forward<Fmt>(ft), std::forward<A>(args)...),
      };
    } LIBIMP_CATCH(std::exception const &e) {
      /// @remark [TBD] std::string constructor may throw an exception
      ctx = {
        level::failed, std::chrono::system_clock::now(), func_, e.what(),
      };
    }
    printer_.output(std::move(ctx));
    return *this;
  }

public:
  grip(char const *func, printer printer = std_out, level level_limit = level::info) noexcept 
    : printer_    (printer)
    , func_       (func)
    , level_limit_(level_limit) {}

  template <typename Fmt, typename... A>
  grip &trace(Fmt &&ft, A &&... args) noexcept {
    return output(log::level::trace, std::forward<Fmt>(ft), std::forward<A>(args)...);
  }
  template <typename Fmt, typename... A>
  grip &debug(Fmt &&ft, A &&... args) noexcept {
    return output(log::level::debug, std::forward<Fmt>(ft), std::forward<A>(args)...);
  }
  template <typename Fmt, typename... A>
  grip &info(Fmt &&ft, A &&... args) noexcept {
    return output(log::level::info, std::forward<Fmt>(ft), std::forward<A>(args)...);
  }
  template <typename Fmt, typename... A>
  grip &warning(Fmt &&ft, A &&... args) noexcept {
    return output(log::level::warning, std::forward<Fmt>(ft), std::forward<A>(args)...);
  }
  template <typename Fmt, typename... A>
  grip &error(Fmt &&ft, A &&... args) noexcept {
    return output(log::level::error, std::forward<Fmt>(ft), std::forward<A>(args)...);
  }
  template <typename Fmt, typename... A>
  grip &failed(Fmt &&ft, A &&... args) noexcept {
    return output(log::level::failed, std::forward<Fmt>(ft), std::forward<A>(args)...);
  }
};

} // namespace log
LIBIMP_NAMESPACE_END_

#define LIBIMP_LOG_(...) ::LIBIMP_::log::grip log {__func__, __VA_ARGS__}