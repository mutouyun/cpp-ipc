/**
 * \file libipc/platform/win/to_tchar.h
 * \author mutouyun (orz@orzz.org)
 */
#pragma once

#include <tchar.h>

#include <string>

#include "libimp/codecvt.h"

#include "libipc/def.h"

LIBIPC_NAMESPACE_BEG_
namespace detail {

using tstring = std::basic_string<TCHAR>;

inline tstring to_tstring(std::string const &str) {
  tstring des;
  ::LIBIMP::cvt_sstr(str, des);
  return des;
}

} // namespace detail
LIBIPC_NAMESPACE_END_