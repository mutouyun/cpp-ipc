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
                ipc::error("fail InitializeSecurityDescriptor[%d]\n", static_cast<int>(::GetLastError()));
                return;
            }
            if (!::SetSecurityDescriptorDacl(&sd_, TRUE, NULL, FALSE)) {
                ipc::error("fail SetSecurityDescriptorDacl[%d]\n", static_cast<int>(::GetLastError()));
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
