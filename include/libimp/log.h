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
  static constexpr char types[] = {
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

/// \brief Standard console output object.
inline auto &make_std_out() noexcept {
  static auto std_out = [](auto const &ctx) {
    auto s = context_to_string(ctx);
    switch (ctx.level) {
    case level::trace:
    case level::debug:
    case level::info:
      std::fprintf(stdout, "%s\n", s.c_str());
      break;
    case level::warning:
    case level::error:
    case level::failed:
      std::fprintf(stderr, "%s\n", s.c_str());
      break;
    default:
      break;
    }
  };
  return std_out;
}

/**
 * \brief Log information base class.
 */
class logger_base {
protected:
  char const *func_;
  level level_limit_;

  logger_base(char const *func, level level_limit) noexcept
    : func_       (func)
    , level_limit_(level_limit) {}
};

/**
 * \brief Log information grips.
 */
template <typename Outputer>
class logger : public logger_base {
  Outputer out_;

public:
  template <typename O>
  logger(char const *func, O &&out, level level_limit) noexcept
    : logger_base(func, level_limit)
    , out_       (std::forward<O>(out)) {}

  template <typename... A>
  logger const &operator()(log::level l, A &&...args) const noexcept {
    if (underlyof(l) < underlyof(level_limit_)) {
      return *this;
    }
    LIBIMP_TRY {
      out_(context<A &&...> {
        l, std::chrono::system_clock::now(), func_,
        std::forward_as_tuple(std::forward<A>(args)...),
      });
    } LIBIMP_CATCH(...) {}
    return *this;
  }

  template <typename... A> logger const &trace  (A &&...args) const noexcept { return (*this)(log::level::trace  , std::forward<A>(args)...); }
  template <typename... A> logger const &debug  (A &&...args) const noexcept { return (*this)(log::level::debug  , std::forward<A>(args)...); }
  template <typename... A> logger const &info   (A &&...args) const noexcept { return (*this)(log::level::info   , std::forward<A>(args)...); }
  template <typename... A> logger const &warning(A &&...args) const noexcept { return (*this)(log::level::warning, std::forward<A>(args)...); }
  template <typename... A> logger const &error  (A &&...args) const noexcept { return (*this)(log::level::error  , std::forward<A>(args)...); }
  template <typename... A> logger const &failed (A &&...args) const noexcept { return (*this)(log::level::failed , std::forward<A>(args)...); }
};

template <typename O>
inline auto make_logger(char const *func, O &&out, level level_limit = level::info) noexcept {
  return logger<std::decay_t<O>>(func, std::forward<O>(out), level_limit);
}

inline auto make_logger(char const *func, level level_limit = level::info) noexcept {
  return make_logger(func, make_std_out(), level_limit);
}

inline auto make_logger(char const * /*ignore*/, char const *name, level level_limit = level::info) noexcept {
  return make_logger(name, make_std_out(), level_limit);
}

} // namespace log
LIBIMP_NAMESPACE_END_

#define LIBIMP_LOG_(...) auto log = ::LIBIMP::log::make_logger(__func__,##__VA_ARGS__)