#include "tls_pointer.h"

#include <windows.h>     // ::Tls...
#include <unordered_map> // std::unordered_map

namespace ipc {

/*
    <Remarks>

    Windows doesn't support a per-thread destructor with its TLS primitives.
    So, here will build it manually by inserting a function to be called on each thread's exit.
    See: https://www.codeproject.com/Articles/8113/Thread-Local-Storage-The-C-Way
         https://src.chromium.org/viewvc/chrome/trunk/src/base/threading/thread_local_storage_win.cc
         https://github.com/mirror/mingw-org-wsl/blob/master/src/libcrt/crt/tlssup.c
         https://github.com/Alexpux/mingw-w64/blob/master/mingw-w64-crt/crt/tlssup.c
         http://svn.boost.org/svn/boost/trunk/libs/thread/src/win32/tss_pe.cpp
*/

namespace {

struct tls_data {
    using destructor_t = void(*)(void*);
    using map_t = std::unordered_map<tls::key_t, tls_data>;

    static DWORD& key() {
        static DWORD rec_key = ::TlsAlloc();
        return rec_key;
    }

    static map_t* records(map_t* rec) {
        ::TlsSetValue(key(), static_cast<LPVOID>(rec));
        return rec;
    }

    static map_t* records() {
        return static_cast<map_t*>(::TlsGetValue(key()));
    }

    tls::key_t   key_        = tls::invalid_value;
    destructor_t destructor_ = nullptr;

    tls_data() = default;

    tls_data(tls::key_t key, destructor_t destructor)
        : key_       (key)
        , destructor_(destructor)
    {}

    tls_data(tls_data&& rhs) : tls_data() {
        (*this) = std::move(rhs);
    }

    tls_data& operator=(tls_data&& rhs) {
        key_            = rhs.key_;
        destructor_     = rhs.destructor_;
        rhs.key_        = 0;
        rhs.destructor_ = nullptr;
        return *this;
    }

    ~tls_data() {
        if (destructor_) destructor_(tls::get(key_));
    }
};

} // internal-linkage

namespace tls {

key_t create(destructor_t destructor) {
    key_t key = static_cast<key_t>(::TlsAlloc());
    if (key == TLS_OUT_OF_INDEXES) return invalid_value;
    auto rec = tls_data::records();
    if (!rec) rec = tls_data::records(new tls_data::map_t);
    if (!rec) return key;
    rec->emplace(key, tls_data{ key, destructor });
    return key;
}

void release(key_t key) {
    auto rec = tls_data::records();
    if (!rec) return;
    rec->erase(key);
    ::TlsFree(static_cast<DWORD>(key));
}

bool set(key_t key, void* ptr) {
    return ::TlsSetValue(static_cast<DWORD>(key),
                         static_cast<LPVOID>(ptr)) == TRUE;
}

void* get(key_t key) {
    return static_cast<void*>(::TlsGetValue(static_cast<DWORD>(key)));
}

} // namespace tls

namespace {

void OnThreadExit() {
    auto rec = tls_data::records();
    if (rec == nullptr) return;
    delete rec;
    tls_data::records(nullptr);
}

void NTAPI OnTlsCallback(PVOID, DWORD dwReason, PVOID) {
    if (dwReason == DLL_THREAD_DETACH) OnThreadExit();
}

} // internal-linkage

////////////////////////////////////////////////////////////////
/// Call destructors on thread exit
////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)

#if defined(IPC_OS_WIN64_)

#pragma comment(linker, "/INCLUDE:_tls_used")
#pragma comment(linker, "/INCLUDE:_tls_xl_b__")

extern "C" {
#   pragma const_seg(".CRT$XLB")
    extern const PIMAGE_TLS_CALLBACK _tls_xl_b__;
    const PIMAGE_TLS_CALLBACK _tls_xl_b__ = OnTlsCallback;
#   pragma const_seg()
}

#else /*!IPC_OS_WIN64_*/

#pragma comment(linker, "/INCLUDE:__tls_used")
#pragma comment(linker, "/INCLUDE:__tls_xl_b__")

extern "C" {
#   pragma data_seg(".CRT$XLB")
    PIMAGE_TLS_CALLBACK _tls_xl_b__ = OnTlsCallback;
#   pragma data_seg()
}

#endif/*!IPC_OS_WIN64_*/

#elif defined(__GNUC__)

#define IPC_CRTALLOC__(x) __attribute__ ((section (x) ))

#if defined(__MINGW64__) || (__MINGW64_VERSION_MAJOR) || \
    (__MINGW32_MAJOR_VERSION > 3) || ((__MINGW32_MAJOR_VERSION == 3) && (__MINGW32_MINOR_VERSION >= 18))

extern "C" {
    IPC_CRTALLOC__(".CRT$XLB") PIMAGE_TLS_CALLBACK _tls_xl_b__ = OnTlsCallback;
}

#else /*!__MINGW*/

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

#endif/*!__MINGW*/

#endif/*_MSC_VER, __GNUC__*/

} // namespace ipc
