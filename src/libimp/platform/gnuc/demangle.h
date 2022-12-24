/**
 * \file libimp/platform/gnuc/demangle.h
 * \author mutouyun (orz@orzz.org)
 */
#pragma once

#include <cxxabi.h> // abi::__cxa_demangle

#include <cstdlib>  // std::malloc

#include "libimp/def.h"
#include "libimp/nameof.h"
#include "libimp/log.h"

LIBIMP_NAMESPACE_BEG_

/**
 * \brief The conventional way to obtain demangled symbol name.
 * \see https://www.boost.org/doc/libs/1_80_0/libs/core/doc/html/core/demangle.html
 * 
 * \param name the mangled name
 * \return std::string a human-readable demangled type name
 */
std::string demangle(span<char const> name) noexcept {
  LIBIMP_LOG_();
  /// \see https://gcc.gnu.org/onlinedocs/libstdc++/libstdc++-html-USERS-4.3/a01696.html
  std::size_t sz = name.size() + 1;
  char *buffer = static_cast<char *>(std::malloc(sz));
  int status = 0;
  char *realname = abi::__cxa_demangle(name.data(), buffer, &sz, &status);
  if (realname == nullptr) {
    log.error("failed: abi::__cxa_demangle(sz = ", sz, "), status = ", status);
    std::free(buffer);
    return {};
  }
  std::string demangled(realname, sz);
  std::free(realname);
  return demangled;
}

LIBIMP_NAMESPACE_END_
