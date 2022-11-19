/**
 * @file libimp/platform/win/system.h
 * @author mutouyun (orz@orzz.org)
 */
#pragma once

#include <exception>
#include <memory>
#include <type_traits>

#include <Windows.h>
#include <tchar.h>

#include "libimp/system.h"
#include "libimp/log.h"
#include "libimp/codecvt.h"
#include "libimp/enum_cast.h"

LIBIMP_NAMESPACE_BEG_
namespace sys {

/**
 * @brief Get the system error code
 * https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
 */
result_code error_code() noexcept {
  auto err = ::GetLastError();
  if (err == ERROR_SUCCESS) {
    return {ERROR_SUCCESS};
  }
  return {false, std::uint64_t(err)};
}

/**
 * @brief Set the system error code
 * https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-setlasterror
 */
void error_code(result_code code) noexcept {
  DWORD err = code ? ERROR_SUCCESS : (DWORD)code.value();
  ::SetLastError(err);
}

/**
 * @brief Gets a text description of the system error
 * https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-formatmessage
 */
std::string error_str(result_code code) noexcept {
  LIBIMP_LOG_();
  try {
    DWORD err = (DWORD)code.value();
    LPTSTR lpErrText = NULL;
    if (::FormatMessage(
          FORMAT_MESSAGE_ALLOCATE_BUFFER | 
          FORMAT_MESSAGE_FROM_SYSTEM | 
          FORMAT_MESSAGE_IGNORE_INSERTS,
          NULL,
          err,
          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
          (LPTSTR)&lpErrText,
          0, NULL) == 0) {
      log.error("failed: FormatMessage(dwMessageId = {}). error = {}", err, error_code());
      return {};
    }
    LIBIMP_UNUSED auto buf_guard = std::unique_ptr<std::remove_pointer_t<LPVOID>, 
                                                   void(*)(LPVOID)> {
      lpErrText, [](LPVOID lp) { ::LocalFree(lp); }
    };
    std::size_t msg_len = ::_tcslen(lpErrText);
    std::size_t len = cvt_cstr(lpErrText, msg_len, (char *)nullptr, 0);
    if (len == 0) {
      return {};
    }
    std::string ret(len, '\0');
    cvt_cstr(lpErrText, msg_len, &ret[0], ret.size());
    return ret;
  } catch (std::exception const &e) {
    log.failed(e.what());
  }
  return {};
}

/**
 * @brief Retrieves information about the current system
 * https://docs.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getsysteminfo
 * https://docs.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getnativesysteminfo
 */
result<std::int64_t> conf(info r) noexcept {
  LIBIMP_LOG_();
  switch (r) {
  case info::page_size: {
    ::SYSTEM_INFO info {};
    ::GetNativeSystemInfo(&info);
    return (std::int64_t)info.dwPageSize;
  }
  default:
    log.error("invalid info = {}", enum_cast(r));
    return {};
  }
}

} // namespace sys
LIBIMP_NAMESPACE_END_
