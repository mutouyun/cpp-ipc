/**
 * @file libimp/platform/posix/system.h
 * @author mutouyun (orz@orzz.org)
 */
#pragma once

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
 * https://manpages.ubuntu.com/manpages/xenial/en/man3/strerror.3.html
 */
std::string error_msg(result_code code) noexcept {
  char msg_buf[256] {};
#if ((_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && !_GNU_SOURCE)
  LIBIMP_LOG_();
  if (::strerror_r((int)code.value(), msg_buf, sizeof(msg_buf)) != 0) {
    log.error("strerror_r fails. return = {}", error_code());
    return {};
  }
  return msg_buf;
#else
  return ::strerror_r((int)code.value(), msg_buf, sizeof(msg_buf));
#endif
}

} // namespace sys
LIBIMP_NAMESPACE_END_
