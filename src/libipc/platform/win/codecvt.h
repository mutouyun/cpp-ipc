/**
 * \file libipc/platform/win/codecvt.h
 * \author mutouyun (orz@orzz.org)
 */
#pragma once

#include <Windows.h>

#include "libipc/imp/codecvt.h"
#include "libipc/imp/detect_plat.h"

namespace ipc {

/**
 * \see https://docs.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar
 *      https://docs.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte
 * 
 *      CP_ACP       : The system default Windows ANSI code page.
 *      CP_MACCP     : The current system Macintosh code page.
 *      CP_OEMCP     : The current system OEM code page.
 *      CP_SYMBOL    : Symbol code page (42).
 *      CP_THREAD_ACP: The Windows ANSI code page for the current thread.
 *      CP_UTF7      : UTF-7. Use this value only when forced by a 7-bit transport mechanism. Use of UTF-8 is preferred.
 *      CP_UTF8      : UTF-8.
*/

template <>
std::size_t cvt_cstr(char const *src, std::size_t slen, wchar_t *des, std::size_t dlen) noexcept {
  if ((src == nullptr) || ((*src) == 0) || (slen == 0)) {
    // source string is empty
    return 0;
  }
  int cch_wc = (des == nullptr) ? 0 : (int)dlen;
  int size_needed = ::MultiByteToWideChar(CP_ACP, 0, src, (int)slen, des, cch_wc);
  if (size_needed <= 0) {
    // failed: MultiByteToWideChar(CP_ACP).
    return 0;
  }
  return size_needed;
}

template <>
std::size_t cvt_cstr(wchar_t const *src, std::size_t slen, char *des, std::size_t dlen) noexcept {
  if ((src == nullptr) || ((*src) == 0) || (slen == 0)) {
    // source string is empty
    return 0;
  }
  int cb_mb = (des == nullptr) ? 0 : (int)dlen;
  int size_needed = ::WideCharToMultiByte(CP_ACP, 0, src, (int)slen, des, cb_mb, NULL, NULL);
  if (size_needed <= 0) {
    // failed: WideCharToMultiByte(CP_ACP).
    return 0;
  }
  return size_needed;
}

/**
 * \brief Used for char8_t (since C++20) to wchar_t conversion.
 * 
 * There is no ut to guarantee correctness (I'm a little lazy here), 
 * so if there are any bugs, please contact me in time.
 */
#if defined(LIBIMP_CPP_20)
template <>
std::size_t cvt_cstr(char8_t const *src, std::size_t slen, wchar_t *des, std::size_t dlen) noexcept {
  if ((src == nullptr) || ((*src) == 0) || (slen == 0)) {
    // source string is empty
    return 0;
  }
  int cch_wc = (des == nullptr) ? 0 : (int)dlen;
  int size_needed = ::MultiByteToWideChar(CP_UTF8, 0, (char *)src, (int)slen, des, cch_wc);
  if (size_needed <= 0) {
    // failed: MultiByteToWideChar(CP_UTF8).
    return 0;
  }
  return size_needed;
}

template <>
std::size_t cvt_cstr(wchar_t const *src, std::size_t slen, char8_t *des, std::size_t dlen) noexcept {
  if ((src == nullptr) || ((*src) == 0) || (slen == 0)) {
    // source string is empty
    return 0;
  }
  int cb_mb = (des == nullptr) ? 0 : (int)dlen;
  int size_needed = ::WideCharToMultiByte(CP_UTF8, 0, src, (int)slen, (char *)des, cb_mb, NULL, NULL);
  if (size_needed <= 0) {
    // failed: WideCharToMultiByte(CP_UTF8).
    return 0;
  }
  return size_needed;
}
#endif // defined(LIBIMP_CPP_20)

} // namespace ipc
