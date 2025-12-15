#pragma once

#include <securitybaseapi.h>

namespace ipc {
namespace detail {

inline LPSECURITY_ATTRIBUTES get_sa() {
    static struct initiator {

        SECURITY_DESCRIPTOR sd_;
        SECURITY_ATTRIBUTES sa_;

        bool succ_ = false;

        initiator() {
            if (!::InitializeSecurityDescriptor(&sd_, SECURITY_DESCRIPTOR_REVISION)) {
                log.error("fail InitializeSecurityDescriptor[", static_cast<int>(::GetLastError(, "]")));
                return;
            }
            if (!::SetSecurityDescriptorDacl(&sd_, TRUE, NULL, FALSE)) {
                log.error("fail SetSecurityDescriptorDacl[", static_cast<int>(::GetLastError(, "]")));
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
} // namespace ipc
