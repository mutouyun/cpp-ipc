
#include <Windows.h>

#include "libipc/platform/tls_detail_win.h"

/**
 * @remarks
 * Windows doesn't support a per-thread destructor with its TLS primitives.
 * So, here will build it manually by inserting a function to be called on each thread's exit.
 *
 * @see
 * - https://www.codeproject.com/Articles/8113/Thread-Local-Storage-The-C-Way
 * - https://src.chromium.org/viewvc/chrome/trunk/src/base/threading/thread_local_storage_win.cc
 * - https://github.com/mirror/mingw-org-wsl/blob/master/src/libcrt/crt/tlssup.c
 * - https://github.com/Alexpux/mingw-w64/blob/master/mingw-w64-crt/crt/tlssup.c
 * - http://svn.boost.org/svn/boost/trunk/libs/thread/src/win32/tss_pe.cpp
*/

namespace ipc {
namespace tls {

namespace {

void NTAPI OnTlsCallback(PVOID, DWORD dwReason, PVOID) {
    if (dwReason == DLL_THREAD_DETACH) {
        ipc::tls::at_thread_exit();
    }
}

} // internal-linkage

////////////////////////////////////////////////////////////////
/// Call destructors on thread exit
////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)

#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__)

#pragma comment(linker, "/INCLUDE:_tls_used")
#pragma comment(linker, "/INCLUDE:_tls_xl_b__")

extern "C" {
#   pragma const_seg(".CRT$XLB")
    extern const PIMAGE_TLS_CALLBACK _tls_xl_b__;
    const PIMAGE_TLS_CALLBACK _tls_xl_b__ = OnTlsCallback;
#   pragma const_seg()
}

#else /*!WIN64*/

#pragma comment(linker, "/INCLUDE:__tls_used")
#pragma comment(linker, "/INCLUDE:__tls_xl_b__")

extern "C" {
#   pragma data_seg(".CRT$XLB")
    PIMAGE_TLS_CALLBACK _tls_xl_b__ = OnTlsCallback;
#   pragma data_seg()
}

#endif/*!WIN64*/

#elif defined(__GNUC__)

#define IPC_CRTALLOC__(x) __attribute__ ((section (x) ))

#if defined(__MINGW64__) || (__MINGW64_VERSION_MAJOR) || \
    (__MINGW32_MAJOR_VERSION > 3) || ((__MINGW32_MAJOR_VERSION == 3) && (__MINGW32_MINOR_VERSION >= 18))

extern "C" {
    IPC_CRTALLOC__(".CRT$XLB") PIMAGE_TLS_CALLBACK _tls_xl_b__ = OnTlsCallback;
}

#else /*!MINGW*/

extern "C" {
    ULONG _tls_index__ = 0;

    IPC_CRTALLOC__(".tls$AAA") char _tls_start__ = 0;
    IPC_CRTALLOC__(".tls$ZZZ") char _tls_end__   = 0;

    IPC_CRTALLOC__(".CRT$XLA") PIMAGE_TLS_CALLBACK _tls_xl_a__ = 0;
    IPC_CRTALLOC__(".CRT$XLB") PIMAGE_TLS_CALLBACK _tls_xl_b__ = OnTlsCallback;
    IPC_CRTALLOC__(".CRT$XLZ") PIMAGE_TLS_CALLBACK _tls_xl_z__ = 0;
}

extern "C" NX_CRTALLOC_(".tls") const IMAGE_TLS_DIRECTORY _tls_used = {
    (ULONG_PTR)(&_tls_start__ + 1),
    (ULONG_PTR) &_tls_end__,
    (ULONG_PTR) &_tls_index__,
    (ULONG_PTR) &_tls_xl_b__,
    (ULONG)0, (ULONG)0
}

#endif/*!MINGW*/

#endif/*_MSC_VER, __GNUC__*/

} // namespace tls
} // namespace ipc
