/**
 * @file libimp/platform/win/codecvt.h
 * @author mutouyun (orz@orzz.org)
 */
#pragma once

#include <Windows.h>

#include "libimp/codecvt.h"
#include "libimp/log.h"

LIBIMP_NAMESPACE_BEG_

/**
 * @brief The transform between local-character-set(UTF-8/GBK/...) and UTF-16/32
 * 
 * https://docs.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar
 * https://docs.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte
 * 
 * CP_ACP       : The system default Windows ANSI code page.
 * CP_MACCP     : The current system Macintosh code page.
 * CP_OEMCP     : The current system OEM code page.
 * CP_SYMBOL    : Symbol code page (42).
 * CP_THREAD_ACP: The Windows ANSI code page for the current thread.
 * CP_UTF7      : UTF-7. Use this value only when forced by a 7-bit transport mechanism. Use of UTF-8 is preferred.
 * CP_UTF8      : UTF-8.
 */

template <>
std::size_t cvt_cstr(char const *src, std::size_t slen, wchar_t *des, std::size_t dlen) noexcept {
  LIBIMP_LOG_();
  int r = ::MultiByteToWideChar(CP_UTF8, 0, src, (int)slen, des, (int)dlen);
  if (r <= 0) {
    log.error("MultiByteToWideChar fails. return = {}", ::GetLastError());
    return 0;
  }
  return (std::size_t)r;
}

template <>
std::size_t cvt_cstr(wchar_t const *src, std::size_t slen, char *des, std::size_t dlen) noexcept {
  LIBIMP_LOG_();
  int r = ::WideCharToMultiByte(CP_UTF8, 0, src, (int)slen, des, (int)dlen, nullptr, nullptr);
  if (r <= 0) {
    log.error("WideCharToMultiByte fails. return = {}", ::GetLastError());
    return 0;
  }
  return (std::size_t)r;
}

LIBIMP_NAMESPACE_END_
