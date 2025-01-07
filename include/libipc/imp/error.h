/**
 * \file libipc/error.h
 * \author mutouyun (orz@orzz.org)
 * \brief A platform-dependent error code.
 */
#pragma once

#include <system_error>
#include <string>
#include <cstdint>

#include "libipc/imp/export.h"
#include "libipc/imp/fmt_cpo.h"

namespace ipc {

/**
 * \brief Custom defined fmt_to method for imp::fmt
 */
namespace detail_tag_invoke {

inline bool tag_invoke(decltype(ipc::fmt_to), fmt_context &ctx, std::error_code const &ec) noexcept {
  return fmt_to(ctx, '[', ec.value(), ": ", ec.message(), ']');
}

} // namespace detail_tag_invoke
} // namespace ipc
