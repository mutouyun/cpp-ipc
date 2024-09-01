/**
 * \file libipc/platform/win/process_impl.h
 * \author mutouyun (orz@orzz.org)
 */
#pragma once

#include <phnt_windows.h>
#include <phnt.h>

#include "libimp/log.h"
#include "libipc/process.h"

LIBIPC_NAMESPACE_BEG_
using namespace ::LIBIMP;

/// \brief Experimental fork() on Windows.
/// \see https://gist.github.com/Cr4sh/126d844c28a7fbfd25c6
///      https://github.com/huntandhackett/process-cloning
namespace {

typedef SSIZE_T pid_t;

pid_t fork() {
  LIBIMP_LOG_();

  RTL_USER_PROCESS_INFORMATION process_info;
  NTSTATUS status;

  /* lets do this */
  status = RtlCloneUserProcess(RTL_CLONE_PROCESS_FLAGS_INHERIT_HANDLES
                             , NULL, NULL, NULL, &process_info);
  if (status == STATUS_PROCESS_CLONED) {
    // Executing inside the clone...
    // Re-attach to the parent's console to be able to write to it
    FreeConsole();
    AttachConsole(ATTACH_PARENT_PROCESS);
    return 0;
  } else {
    // Executing inside the original (parent) process...
    if (!NT_SUCCESS(status)) {
      log.error("failed: RtlCloneUserProcess(...)");
      return -1;
    }
    return (pid_t)process_info.ProcessHandle;
  }

  /* NOTREACHED */
}

#define	WNOHANG 1	/* Don't block waiting. */

/// \see https://man7.org/linux/man-pages/man3/wait.3p.html
///      https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntwaitforsingleobject
pid_t waitpid(pid_t pid, int */*status*/, int options) {
  LIBIMP_LOG_();
  if (pid == -1) {
    return -1;
  }
  if (options & WNOHANG) {
    return pid;
  }
  NTSTATUS status = NtWaitForSingleObject((HANDLE)pid, FALSE, NULL);
  if (!NT_SUCCESS(status)) {
    log.error("failed: NtWaitForSingleObject(...)");
    return -1;
  }
  return pid;
}

} // namespace
LIBIPC_NAMESPACE_END_
