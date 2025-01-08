/**
 * \file libipc/system.h
 * \author mutouyun (orz@orzz.org)
 * \brief Isolation and encapsulation of system APIs.
 */
#pragma once

#include <string>
#include <ostream>  // std::ostream
#include <cstdint>

#include "libipc/imp/export.h"
#include "libipc/imp/error.h"
#include "libipc/imp/result.h"

namespace ipc {
namespace sys {

/// \brief A platform-dependent error code.
LIBIPC_EXPORT std::error_code error() noexcept;

/// \enum The name of the `conf()` argument used to inquire about its value.
/// \brief Certain options are supported, 
/// or what the value is of certain configurable constants or limits.
enum class info : std::int32_t {
  page_size,
};

/// \brief Get system configuration information at run time.
LIBIPC_EXPORT result<std::int64_t> conf(info) noexcept;

} // namespace sys
} // namespace ipc
