/**
 * \file libipc/platform/posix/mmap.h
 * \author mutouyun (orz@orzz.org)
 */
#pragma once

#include "libipc/def.h"

LIBIPC_NAMESPACE_BEG_
namespace posix {

enum : int {
  succ   = 0,
  failed = -1,
};

} // namespace posix
LIBIPC_NAMESPACE_END_
