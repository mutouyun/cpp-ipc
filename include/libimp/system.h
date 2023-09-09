/**
 * \file libimp/system.h
 * \author mutouyun (orz@orzz.org)
 * \brief Isolation and encapsulation of system APIs.
 * \date 2022-08-07
 */
#pragma once

#include <string>
#include <ostream>  // std::ostream
#include <cstdint>

#include "libimp/def.h"
#include "libimp/export.h"
#include "libimp/error.h"
#include "libimp/result.h"

LIBIMP_NAMESPACE_BEG_
namespace sys {

/// \brief A platform-dependent error code.
LIBIMP_EXPORT std::error_code error() noexcept;

/// \enum The name of the `conf()` argument used to inquire about its value.
/// \brief Certain options are supported, 
/// or what the value is of certain configurable constants or limits.
enum class info : std::int32_t {
  page_size,
};

/// \brief Get system configuration information at run time.
LIBIMP_EXPORT result<std::int64_t> conf(info) noexcept;

} // namespace sys
LIBIMP_NAMESPACE_END_
