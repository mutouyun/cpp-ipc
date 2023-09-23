#pragma once

#include <Windows.h>

#include "libipc/memory/resource.h"

namespace ipc {
namespace detail {

/// \brief This routine returns `true` if the caller's process is a member of the Administrators local group. 
///        Caller is NOT expected to be impersonating anyone and is expected to be able to open its own process and process token.
/// \return true  - Caller has Administrators local group.
///         false - Caller does not have Administrators local group.
/// \see https://learn.microsoft.com/en-us/windows/win32/api/securitybaseapi/nf-securitybaseapi-checktokenmembership
inline bool is_user_admin() {
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup; 
    BOOL b = AllocateAndInitializeSid(&NtAuthority,
                                      2,
                                      SECURITY_BUILTIN_DOMAIN_RID,
                                      DOMAIN_ALIAS_RID_ADMINS,
                                      0, 0, 0, 0, 0, 0,
                                      &AdministratorsGroup); 
    if (b) {
        if (!CheckTokenMembership(NULL, AdministratorsGroup, &b)) {
            b = FALSE;
        }
        FreeSid(AdministratorsGroup); 
    }
    return !!(b);
}

/// \brief Identify the user group and add the appropriate prefix to the string.
/// \see http://msdn.microsoft.com/en-us/library/aa366551(v=VS.85).aspx
///      https://stackoverflow.com/questions/3999157/system-error-0x5-createfilemapping
inline ipc::string make_comfortable_prefix(ipc::string &&txt) {
    if (is_user_admin()) {
        return ipc::string{"Global\\"} + txt;
    }
    return txt;
}

} // namespace detail
} // namespace ipc
