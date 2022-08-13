/**
 * @file libimp/platform/posix/system.h
 * @author mutouyun (orz@orzz.org)
 */
#pragma once

#include <exception>
#include <memory>
#include <type_traits>

#include <errno.h>
#include <string.h>

#include "libimp/system.h"
#include "libimp/log.h"

LIBIMP_NAMESPACE_BEG_
namespace sys {

/**
 * @brief Get the system error code
 * https://man7.org/linux/man-pages/man3/errno.3.html
 */
result_code error_code() noexcept {
  auto err = errno;
  if (err == 0) return {true};
  return {false, std::uint64_t(err)};
}

/**
 * @brief Set the system error code
 * https://man7.org/linux/man-pages/man3/errno.3.html
 */
void error_code(result_code code) noexcept {
  errno = code ? 0 : (int)code.value();
}

/**
 * @brief Gets a text description of the system error
 * https://man7.org/linux/man-pages/man3/strerror_l.3.html
 */
std::string error_msg(result_code code) noexcept {
  return {};
}

} // namespace sys
LIBIMP_NAMESPACE_END_
