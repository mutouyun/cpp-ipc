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

LIBIMP_NAMESPACE_BEG_
namespace sys {

/**
 * @brief Get the system error code
 * https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
 */
result_code error_code() noexcept {
  auto err = ::GetLastError();
  if (err == ERROR_SUCCESS) return {true};
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
std::string error_msg(result_code code) noexcept {
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
      log.error("FormatMessage fails. return = {}", error_code());
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

} // namespace sys
LIBIMP_NAMESPACE_END_
