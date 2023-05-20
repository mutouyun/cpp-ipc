/**
 * \file libimp/log.h
 * \author mutouyun (orz@orzz.org)
 * \brief Simple log output component.
 * \date 2022-05-22
 */
#pragma once

#include <string>
#include <type_traits>
#include <chrono>
#include <tuple>
#include <utility>

#include "libimp/def.h"
#include "libimp/detect_plat.h"
#include "libimp/export.h"
#include "libimp/underlyof.h"
#include "libimp/fmt.h"
#include "libimp/generic.h"

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

/// \struct template <typename... T> struct context
/// \brief Logging context.
/// \tparam ...T - a log records all parameter types passed
template <typename... T>
struct context {
  log::level level;
  std::chrono::system_clock::time_point tp;
  char const *func;
  std::tuple<T...> params;
};

/// \brief Custom defined fmt_to method for imp::fmt
namespace detail {

template <typename Tp, std::size_t... I, typename... A>
bool unfold_tuple_fmt_to(fmt_context &ctx, Tp const &tp, std::index_sequence<I...>, A &&...args) {
  return fmt_to(ctx, std::forward<A>(args)..., std::get<I>(tp)...);
}

} // namespace detail

template <typename... T>
bool context_to_string(fmt_context &f_ctx, context<T...> const &l_ctx) noexcept {
  constexpr static char types[] = {
    'T', 'D', 'I', 'W', 'E', 'F',
  };
  LIBIMP_TRY {
    auto ms = std::chrono::time_point_cast<std::chrono::milliseconds>(l_ctx.tp).time_since_epoch().count() % 1000;
    return detail::unfold_tuple_fmt_to(f_ctx, l_ctx.params, std::index_sequence_for<T...>{},
                                      "[", types[underlyof(l_ctx.level)], "]"
                                      "[", l_ctx.tp, ".", spec("03")(ms), "]"
                                      "[", l_ctx.func, "] ");
  } LIBIMP_CATCH(...) {
    return false;
  }
}

template <typename... T>
std::string context_to_string(context<T...> const &l_ctx) noexcept {
  LIBIMP_TRY {
    std::string log_txt;
    fmt_context f_ctx(log_txt);
    if (!context_to_string(f_ctx, l_ctx)) {
      return {};
    }
    f_ctx.finish();
    return log_txt;
  } LIBIMP_CATCH(...) {
    return {};
  }
}

namespace detail {

enum out_type : unsigned {
  out_none    = 0x0,
  out_string  = 0x1,
};

template <typename T>
class has_fn_output {
  static std::integral_constant<out_type, out_none> check(...);

  template <typename U, typename = decltype(std::declval<U &>().output(log::level::trace, std::declval<std::string>()))>
  static std::integral_constant<out_type, out_string> check(U *u);

public:
  using type = decltype(check(static_cast<T *>(nullptr)));
};

template <typename T>
constexpr out_type has_fn_output_v = has_fn_output<T>::type::value;

struct vtable_t {
  void (*output)(void *, log::level, std::string &&);
};

template <typename T>
class traits {
  template <typename U>
  static auto make_fn_output() noexcept
    -> std::enable_if_t<(has_fn_output_v<U> == out_none), decltype(vtable_t{}.output)> {
    return [](void *, log::level, std::string &&) {};
  }

  template <typename U>
  static auto make_fn_output() noexcept
    -> std::enable_if_t<(has_fn_output_v<U> == out_string), decltype(vtable_t{}.output)> {
    return [](void *p, log::level lev, std::string &&str) {
      static_cast<U *>(p)->output(lev, std::move(str));
    };
  }

public:
  static auto make_vtable() noexcept {
    static vtable_t vt { make_fn_output<T>() };
    return &vt;
  }
};

/// \brief Custom defined fmt_to method for imp::fmt
template <typename... T>
bool tag_invoke(decltype(::LIBIMP::fmt_to), fmt_context &f_ctx, context<T...> const &l_ctx) noexcept {
  return ::LIBIMP::log::context_to_string(f_ctx, l_ctx);
}

} // namespace detail

class printer {
  void *objp_ {nullptr};
  detail::vtable_t *vtable_ {nullptr};

public:
  printer() noexcept = default;

  template <typename T, 
            typename = is_not_match<printer, T>>
  printer(T &p) noexcept
    : objp_  (static_cast<void *>(&p))
    , vtable_(detail::traits<T>::make_vtable()) {}

  explicit operator bool() const noexcept {
    return (objp_ != nullptr) && (vtable_ != nullptr);
  }

  template <typename... T>
  void output(context<T...> const &ctx) noexcept {
    if (!*this) return;
    LIBIMP_TRY {
      vtable_->output(objp_, ctx.level, context_to_string(ctx));
    } LIBIMP_CATCH(...) {}
  }
};

/// \class class LIBIMP_EXPORT std_t
/// \brief Standard console output.
class LIBIMP_EXPORT std_t {
public:
  void output(log::level, std::string &&) noexcept;
};

/// \brief Standard console output object.
LIBIMP_EXPORT extern std_t std_out;

/**
 * \brief Log information grips.
 */
class grip {
  printer printer_;
  char const *func_;
  level level_limit_;

  template <typename... A>
  grip &output(log::level l, A &&... args) noexcept {
    if (!printer_ || (underlyof(l) < underlyof(level_limit_))) {
      return *this;
    }
    printer_.output(context<A &&...> {
      l, std::chrono::system_clock::now(), func_,
      std::forward_as_tuple(std::forward<A>(args)...),
    });
    return *this;
  }

public:
  grip(char const *func, printer printer = std_out, level level_limit = level::info) noexcept 
    : printer_    (printer)
    , func_       (func)
    , level_limit_(level_limit) {}

  template <typename... A> grip &trace  (A &&...args) noexcept { return output(log::level::trace  , std::forward<A>(args)...); }
  template <typename... A> grip &debug  (A &&...args) noexcept { return output(log::level::debug  , std::forward<A>(args)...); }
  template <typename... A> grip &info   (A &&...args) noexcept { return output(log::level::info   , std::forward<A>(args)...); }
  template <typename... A> grip &warning(A &&...args) noexcept { return output(log::level::warning, std::forward<A>(args)...); }
  template <typename... A> grip &error  (A &&...args) noexcept { return output(log::level::error  , std::forward<A>(args)...); }
  template <typename... A> grip &failed (A &&...args) noexcept { return output(log::level::failed , std::forward<A>(args)...); }
};

} // namespace log
LIBIMP_NAMESPACE_END_

#define LIBIMP_LOG_(...) ::LIBIMP::log::grip log(__func__,##__VA_ARGS__)