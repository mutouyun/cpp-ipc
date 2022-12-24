/**
 * \file libimp/fmt_cpo.h
 * \author mutouyun (orz@orzz.org)
 * \brief String formatting CPO.
 * \date 2022-11-28
 */
#pragma once

#include <string>
#include <cstddef>

#include "libimp/def.h"
#include "libimp/generic.h"
#include "libimp/detect_plat.h"
#include "libimp/span.h"
#include "libimp/export.h"

LIBIMP_NAMESPACE_BEG_

/**
 * \brief The context of fmt.
 */
class LIBIMP_EXPORT fmt_context {
  std::string &joined_;
  std::size_t  offset_;

public:
  fmt_context(std::string &j) noexcept;

  std::size_t capacity() noexcept;
  void reset() noexcept;
  bool finish() noexcept;
  span<char> buffer(std::size_t sz) noexcept;
  void expend(std::size_t sz) noexcept;
  bool append(std::string const &str) noexcept;
};

/**
 * \brief Supports custom fmt_to methods for imp::fmt.
 */
namespace detail {

class fmt_to_t {
  template <typename A1>
  bool get_result(fmt_context &ctx, A1 && a1) const {
    return ::LIBIMP::tag_invoke(fmt_to_t{}, ctx, std::forward<A1>(a1));
  }

  template <typename A1, typename... A>
  bool get_result(fmt_context &ctx, A1 && a1, A &&...args) const {
    return get_result(ctx, std::forward<A1>(a1))
        && get_result(ctx, std::forward<A>(args)...);
  }

public:
  template <typename... A>
  bool operator()(fmt_context &ctx, A &&...args) const {
    return get_result(ctx, std::forward<A>(args)...);
  }
};

} // namespace detail

constexpr detail::fmt_to_t fmt_to {};

LIBIMP_NAMESPACE_END_
