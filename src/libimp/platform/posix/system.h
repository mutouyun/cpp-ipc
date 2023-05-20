/**
 * \file libimp/platform/posix/system.h
 * \author mutouyun (orz@orzz.org)
 */
#pragma once

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "libimp/system.h"
#include "libimp/log.h"

LIBIMP_NAMESPACE_BEG_
namespace sys {

#ifndef ENOERR
#define ENOERR (0)
#endif

/**
 * \brief Get the system error number
 * https://man7.org/linux/man-pages/man3/errno.3.html
 */
error_code_t error_no() noexcept {
  auto err = errno;
  if (err == ENOERR) return {ENOERR};
  return error_code_t(err);
}

/**
 * \brief Set the system error number
 * https://man7.org/linux/man-pages/man3/errno.3.html
 */
void error_no(error_code_t const &code) noexcept {
  errno = (code == 0) ? ENOERR : (int)code;
}

/**
 * \brief Gets a text description of the system error
 * https://man7.org/linux/man-pages/man3/strerror_l.3.html
 * https://manpages.ubuntu.com/manpages/xenial/en/man3/strerror.3.html
 */
std::string error_str(error_code_t const &code) noexcept {
  char msg_buf[256] {};
#if ((_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && !_GNU_SOURCE)
  LIBIMP_LOG_();
  if (::strerror_r((int)code, msg_buf, sizeof(msg_buf)) != 0) {
    log.error("failed: strerror_r(code = ", (int)code, 
                      ", buf, buf-size = ", sizeof(msg_buf), 
                      "). error = ", error_code());
    return {};
  }
  return msg_buf;
#else
  return ::strerror_r((int)code, msg_buf, sizeof(msg_buf));
#endif
}

/**
 * \brief Gets configuration information at run time
 * https://man7.org/linux/man-pages/man2/getpagesize.2.html
 * https://man7.org/linux/man-pages/man3/sysconf.3.html
 */
result<std::int64_t> conf(info r) noexcept {
  LIBIMP_LOG_();
  switch (r) {
  case info::page_size: {
    auto val = ::sysconf(_SC_PAGESIZE);
    if (val >= 0) return (std::int64_t)val;
    break;
  }
  default:
    log.error("invalid info = ", underlyof(r));
    return {};
  }
  auto err = sys::error();
  log.error("info = ", underlyof(r), ", error = ", err);
  return {false, (int)err.code()};
}

} // namespace sys
LIBIMP_NAMESPACE_END_
