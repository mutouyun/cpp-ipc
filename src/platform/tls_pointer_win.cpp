#include "tls_pointer.h"
#include "log.h"

#include <Windows.h>     // ::Tls...
#include <atomic>
#include <unordered_set> // std::unordered_set

namespace ipc {

/*
 * <Remarks>
 *
 * Windows doesn't support a per-thread destructor with its TLS primitives.
 * So, here will build it manually by inserting a function to be called on each thread's exit.
 *
 * <Reference>
 * - https://www.codeproject.com/Articles/8113/Thread-Local-Storage-The-C-Way
 * - https://src.chromium.org/viewvc/chrome/trunk/src/base/threading/thread_local_storage_win.cc
 * - https://github.com/mirror/mingw-org-wsl/blob/master/src/libcrt/crt/tlssup.c
 * - https://github.com/Alexpux/mingw-w64/blob/master/mingw-w64-crt/crt/tlssup.c
 * - http://svn.boost.org/svn/boost/trunk/libs/thread/src/win32/tss_pe.cpp
*/

namespace {

struct tls_data {
    using destructor_t = void(*)(void*);

    DWORD        win_key_;
    destructor_t destructor_;

    void destruct(void* data) {
        if ((destructor_ != nullptr) && (data != nullptr)) {
            destructor_(data);
        }
    }
};

using rec_t = std::unordered_set<tls_data*>;

DWORD& record_key() {

    struct key_gen {
        DWORD rec_key_;
        key_gen() : rec_key_(::TlsAlloc()) {
            if (rec_key_ == TLS_OUT_OF_INDEXES) {
                ipc::error("[record_key] TlsAlloc failed[%lu].\n", ::GetLastError());
            }
        }
        ~key_gen() { ::TlsFree(rec_key_); }
    };

    static key_gen gen;
    return gen.rec_key_;
}

bool record(tls_data* tls) {
    auto rec = static_cast<rec_t*>(::TlsGetValue(record_key()));
    if (rec == nullptr) {
        if (FALSE == ::TlsSetValue(record_key(), static_cast<LPVOID>(rec = new rec_t))) {
            ipc::error("[record] TlsSetValue failed[%lu].\n", ::GetLastError());
            return false;
        }
    }
    rec->insert(tls);
    return true;
}

static void erase_record(tls_data* tls) {
    auto rec = static_cast<rec_t*>(::TlsGetValue(record_key()));
    if (rec == nullptr) return;
    rec->erase(tls);
}

static void clear_all_records() {
    auto rec = static_cast<rec_t*>(::TlsGetValue(record_key()));
    if (rec == nullptr) return;
    for (auto tls : *rec) {
        if (tls != nullptr) {
            tls->destruct(::TlsGetValue(tls->win_key_));
        }
    }
    delete rec;
    ::TlsSetValue(record_key(), static_cast<LPVOID>(nullptr));
}

} // internal-linkage

namespace tls {

key_t create(destructor_t destructor) {
    record_key(); // gen record-key
    auto tls_dat = new tls_data { ::TlsAlloc(), destructor };
    std::atomic_thread_fence(std::memory_order_seq_cst);
    if (tls_dat->win_key_ == TLS_OUT_OF_INDEXES) {
        ipc::error("[tls::create] TlsAlloc failed[%lu].\n", ::GetLastError());
        delete tls_dat;
        return invalid_value;
    }
    return reinterpret_cast<key_t>(tls_dat);
}

void release(key_t tls_key) {
    if (tls_key == invalid_value) {
        ipc::error("[tls::release] tls_key is invalid_value.\n");
        return;
    }
    auto tls_dat = reinterpret_cast<tls_data*>(tls_key);
    if (tls_dat == nullptr) {
        ipc::error("[tls::release] tls_dat is nullptr.\n");
        return;
    }
    erase_record(tls_dat);
    ::TlsFree(tls_dat->win_key_);
    delete tls_dat;
}

bool set(key_t tls_key, void* ptr) {
    if (tls_key == invalid_value) {
        ipc::error("[tls::set] tls_key is invalid_value.\n");
        return false;
    }
    auto tls_dat = reinterpret_cast<tls_data*>(tls_key);
    if (tls_dat == nullptr) {
        ipc::error("[tls::set] tls_dat is nullptr.\n");
        return false;
    }
    if (FALSE == ::TlsSetValue(tls_dat->win_key_, static_cast<LPVOID>(ptr))) {
        ipc::error("[tls::set] TlsSetValue failed[%lu].\n", ::GetLastError());
        return false;
    }
    record(tls_dat);
    return true;
}

void* get(key_t tls_key) {
    if (tls_key == invalid_value) {
        ipc::error("[tls::get] tls_key is invalid_value.\n");
        return nullptr;
    }
    auto tls_dat = reinterpret_cast<tls_data*>(tls_key);
    if (tls_dat == nullptr) {
        ipc::error("[tls::get] tls_dat is nullptr.\n");
        return nullptr;
    }
    return ::TlsGetValue(tls_dat->win_key_);
}

} // namespace tls

namespace {

void OnThreadExit() {
    clear_all_records();
}

void NTAPI OnTlsCallback(PVOID, DWORD dwReason, PVOID) {
    if (dwReason == DLL_THREAD_DETACH) OnThreadExit();
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

} // namespace ipc
