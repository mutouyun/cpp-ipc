/**
 * @file libipc/platform/win/get_sa.h
 * @author mutouyun (orz@orzz.org)
 */
#pragma once

#include <securitybaseapi.h>

#include "libimp/system.h"
#include "libimp/log.h"

#include "libipc/def.h"

LIBIPC_NAMESPACE_BEG_
namespace detail {

/**
 * @brief Create a SECURITY_ATTRIBUTES structure singleton
 * @see https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/aa379560(v=vs.85)
 */
inline LPSECURITY_ATTRIBUTES get_sa() {
  static struct initiator {

    SECURITY_DESCRIPTOR sd_;
    SECURITY_ATTRIBUTES sa_;

    bool succ_ = false;

    initiator() {
      using namespace ::LIBIMP_;
      log::gripper log {"get_sa"};
      if (!::InitializeSecurityDescriptor(&sd_, SECURITY_DESCRIPTOR_REVISION)) {
        log.error("InitializeSecurityDescriptor fails. error = {}", sys::error());
        return;
      }
      if (!::SetSecurityDescriptorDacl(&sd_, TRUE, NULL, FALSE)) {
        log.error("SetSecurityDescriptorDacl fails. error = {}", sys::error());
        return;
      }
      sa_.nLength = sizeof(SECURITY_ATTRIBUTES);
      sa_.bInheritHandle = FALSE;
      sa_.lpSecurityDescriptor = &sd_;
      succ_ = true;
    }
  } handle;
  return handle.succ_ ? &handle.sa_ : nullptr;
}

} // namespace detail
LIBIPC_NAMESPACE_END_