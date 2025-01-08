/**
 * \file libipc/platform/win/system.h
 * \author mutouyun (orz@orzz.org)
 */
#pragma once

#include <exception>
#include <type_traits>

#include <Windows.h>
#include <tchar.h>

#include "libipc/imp/system.h"
#include "libipc/imp/log.h"
#include "libipc/imp/codecvt.h"
#include "libipc/imp/generic.h"
#include "libipc/imp/detect_plat.h"
#include "libipc/imp/scope_exit.h"

namespace ipc {
namespace sys {

/**
 * \brief Gets a text description of the system error
 * \see https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-formatmessage
 */
std::string error_string(DWORD code) noexcept {
  LIBIPC_LOG();
  LIBIPC_TRY {
    LPTSTR lpErrText = NULL;
    if (::FormatMessage(
          FORMAT_MESSAGE_ALLOCATE_BUFFER | 
          FORMAT_MESSAGE_FROM_SYSTEM | 
          FORMAT_MESSAGE_IGNORE_INSERTS,
          NULL,
          code,
          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
          (LPTSTR)&lpErrText,
          0, NULL) == 0) {
      log.error("failed: FormatMessage(dwMessageId = ", code, "). error = ", ::GetLastError());
      return {};
    }
    LIBIPC_SCOPE_EXIT(finally) = [lpErrText] { ::LocalFree(lpErrText); };
    std::size_t msg_len = ::_tcslen(lpErrText);
    std::size_t len = cvt_cstr(lpErrText, msg_len, (char *)nullptr, 0);
    if (len == 0) {
      return {};
    }
    std::string ret(len, '\0');
    cvt_cstr(lpErrText, msg_len, &ret[0], ret.size());
    return ret;
  } LIBIPC_CATCH(...) {
    log.error("failed: FormatMessage(dwMessageId = ", code, ").",
              "\n\texception: ", log::exception_string(std::current_exception()));
  }
  return {};
}

/**
 * \brief Get the system error number.
 * \see https://en.cppreference.com/w/cpp/error/system_category
 *      https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
*/
std::error_code error() noexcept {
  return std::error_code(::GetLastError(), std::system_category());
}

/**
 * \brief Retrieves information about the current system.
 * \see https://docs.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getsysteminfo
 *      https://docs.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getnativesysteminfo
 */
result<std::int64_t> conf(info r) noexcept {
  LIBIPC_LOG();
  switch (r) {
  case info::page_size: {
    ::SYSTEM_INFO info{};
    ::GetNativeSystemInfo(&info);
    return (std::int64_t)info.dwPageSize;
  }
  default:
    log.error("invalid info = ", underlyof(r));
    return std::make_error_code(std::errc::invalid_argument);
  }
}

} // namespace sys
} // namespace ipc
