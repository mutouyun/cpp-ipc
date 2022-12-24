/**
 * \file libimp/platform/win/demangle.h
 * \author mutouyun (orz@orzz.org)
 */
#pragma once

#include "libimp/def.h"
#include "libimp/nameof.h"

LIBIMP_NAMESPACE_BEG_

std::string demangle(span<char const> name) noexcept {
  return std::string(name.data(), name.size());
}

LIBIMP_NAMESPACE_END_
