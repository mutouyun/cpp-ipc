/**
 * \file libipc/platform/win/mutex_impl.h
 * \author mutouyun (orz@orzz.org)
 */
#pragma once

#include "libimp/log.h"
#include "libimp/system.h"
#include "libpmr/new.h"
#include "libipc/mutex.h"

#include "api.h"
#include "sync_id.h"

LIBIPC_NAMESPACE_BEG_

using namespace ::LIBIMP;
using namespace ::LIBPMR;

struct mutex_handle {

};

result<mutex_t> mutex_open(span<::LIBIMP::byte> mem) noexcept {
  return {};
}

LIBIPC_NAMESPACE_END_
