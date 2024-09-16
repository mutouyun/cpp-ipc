/**
 * \file libipc/platform/win/close_handle.h
 * \author mutouyun (orz@orzz.org)
 */
#pragma once

#include <Windows.h>

#include "libimp/log.h"
#include "libimp/system.h"

#include "libipc/def.h"

LIBIPC_NAMESPACE_BEG_
using namespace ::LIBIMP;

namespace winapi {

/**
 * \brief Closes an open object handle.
 * \see https://docs.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-closehandle
 */
inline result<void> close_handle(HANDLE h) noexcept {
  LIBIMP_LOG_();
  if (h == NULL) {
    log.error("handle is null.");
    return std::make_error_code(std::errc::invalid_argument);
  }
  if (!::CloseHandle(h)) {
    auto err = sys::error();
    log.error("failed: CloseHandle(", h, "). error = ", err);
    return err;
  }
  return std::error_code{};
}

} // namespace winapi
LIBIPC_NAMESPACE_END_
