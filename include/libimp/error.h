/**
 * \file libimp/error.h
 * \author mutouyun (orz@orzz.org)
 * \brief A platform-dependent error code.
 * \date 2022-12-18
 */
#pragma once

#include <system_error>
#include <string>
#include <cstdint>

#include "libimp/def.h"
#include "libimp/export.h"
#include "libimp/fmt_cpo.h"

LIBIMP_NAMESPACE_BEG_

/**
 * \brief Custom defined fmt_to method for imp::fmt
 */
namespace detail {

inline bool tag_invoke(decltype(::LIBIMP::fmt_to), fmt_context &ctx, std::error_code const &ec) noexcept {
  return fmt_to(ctx, '[', ec.value(), ": ", ec.message(), ']');
}

} // namespace detail
LIBIMP_NAMESPACE_END_
