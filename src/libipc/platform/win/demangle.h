/**
 * \file libipc/platform/win/demangle.h
 * \author mutouyun (orz@orzz.org)
 */
#pragma once

#include "libipc/imp/nameof.h"

namespace ipc {

std::string demangle(std::string name) noexcept {
  return std::move(name);
}

} // namespace ipc
