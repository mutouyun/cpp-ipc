/**
 * \file libipc/fmt_cpo.h
 * \author mutouyun (orz@orzz.org)
 * \brief String formatting CPO.
 */
#pragma once

#include <string>
#include <cstddef>
#include <array>

#include "libipc/imp/generic.h"
#include "libipc/imp/detect_plat.h"
#include "libipc/imp/span.h"
#include "libipc/imp/export.h"

namespace ipc {

/**
 * \class class LIBIPC_EXPORT fmt_context
 * \brief The context of fmt.
 */
class LIBIPC_EXPORT fmt_context {
  std::array<char, 2048U> sbuf_; ///< stack buffer

  std::string &joined_;
  std::size_t  offset_;

public:
  fmt_context(std::string &j) noexcept;

  std::size_t capacity() noexcept;
  void reset() noexcept;
  bool finish() noexcept;
  span<char> buffer(std::size_t sz) noexcept;
  void expend(std::size_t sz) noexcept;
  bool append(span<char const> const &str) noexcept;
};

/// \brief Supports custom fmt_to methods for ipc::fmt.
namespace detail_tag_invoke {

class fmt_to_t {
  template <typename A1>
  bool get_result(fmt_context &ctx, A1 && a1) const {
    return ipc::tag_invoke(fmt_to_t{}, ctx, std::forward<A1>(a1));
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

} // namespace detail_tag_invoke

constexpr detail_tag_invoke::fmt_to_t fmt_to{};

} // namespace ipc
