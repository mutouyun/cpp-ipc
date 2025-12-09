/**
 * \file libipc/platform/posix/system.h
 * \author mutouyun (orz@orzz.org)
 */
#pragma once

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "libipc/imp/system.h"
#include "libipc/imp/log.h"

namespace ipc {
namespace sys {

/**
 * \brief Get the system error number.
 * \see https://en.cppreference.com/w/cpp/error/generic_category
 *      https://man7.org/linux/man-pages/man3/errno.3.html
*/
std::error_code error() noexcept {
  return std::error_code(errno, std::generic_category());
}

/**
 * \brief Gets configuration information at run time
 * https://man7.org/linux/man-pages/man2/getpagesize.2.html
 * https://man7.org/linux/man-pages/man3/sysconf.3.html
 */
result<std::int64_t> conf(info r) noexcept {
  LIBIPC_LOG();
  switch (r) {
  case info::page_size: {
    auto val = ::sysconf(_SC_PAGESIZE);
    if (val >= 0) return static_cast<std::int64_t>(val);
    break;
  }
  default:
    log.error("invalid info = ", underlyof(r));
    return std::make_error_code(std::errc::invalid_argument);
  }
  auto err = sys::error();
  log.error("info = ", underlyof(r), ", error = ", err);
  return err;
}

} // namespace sys
} // namespace ipc
