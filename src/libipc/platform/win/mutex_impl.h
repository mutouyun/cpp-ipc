/**
 * \file libipc/platform/win/mutex_impl.h
 * \author mutouyun (orz@orzz.org)
 */
#pragma once

#include "libimp/log.h"
#include "libimp/system.h"
#include "libipc/mutex.h"

#include "api.h"

LIBIPC_NAMESPACE_BEG_

using namespace ::LIBIMP;

struct mutex_handle {

};

namespace winapi {

/**
 * \brief Creates or opens a named or unnamed mutex object.
 * \see https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createmutexa
 * \return Mutex object HANDLE, NULL on error.
 */
result<HANDLE> mutex_open_or_create(char const *name, bool initial_owner) noexcept {
  LIBIMP_LOG_();
  HANDLE h = ::CreateMutexA(winapi::get_sa(), initial_owner, name);
  if (h == NULL) {
    auto err = sys::error();
    log.error("failed: CreateMutexA(", initial_owner, ", ", name, "). error = ", err);
    return err;
  }
  return h;
}

/**
 * \brief Releases ownership of the specified mutex object.
 * \see https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-releasemutex
*/
result<bool> mutex_release(HANDLE h) noexcept {
  LIBIMP_LOG_();
  if (::ReleaseMutex(h)) {
    return true;
  }
  auto err = sys::error();
  log.error("failed: ReleaseMutex. error = ", err);
  return err;
}

/**
 * \brief Locks the mutex, blocks if the mutex is not available.
 * \see https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
*/
result<bool> mutex_wait(HANDLE h, std::int64_t ms) noexcept {
  LIBIMP_LOG_();
  for (;;) {
    auto r = winapi::wait_for_single_object(h, ms);
    if (!r) {
      return r.error();
    }
    if (*r == winapi::wait_result::object_0) {
      return true;
    }
    if (*r == winapi::wait_result::abandoned) {
      log.info("failed: WaitForSingleObject(", ms, "). The mutex is abandoned, try again.");
      auto rr = mutex_release(h);
      if (rr) {
        continue;
      }
      return rr.error();
    }
    return false;
  }
}

} // namespace winapi

result<mutex_t> mutex_open(span<::LIBIMP::byte> mem) noexcept {
  return {};
}

LIBIPC_NAMESPACE_END_
