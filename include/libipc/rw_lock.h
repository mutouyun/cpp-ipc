#pragma once

#include <atomic>
#include <thread>
#include <chrono>
#include <limits>
#include <type_traits>
#include <utility>

////////////////////////////////////////////////////////////////
/// Gives hint to processor that improves performance of spin-wait loops.
////////////////////////////////////////////////////////////////

#pragma push_macro("IPC_LOCK_PAUSE_")
#undef  IPC_LOCK_PAUSE_

#if defined(_MSC_VER)
#include <windows.h>    // YieldProcessor
/*
    See: http://msdn.microsoft.com/en-us/library/windows/desktop/ms687419(v=vs.85).aspx
    Not for intel c++ compiler, so ignore http://software.intel.com/en-us/forums/topic/296168
*/
#   define IPC_LOCK_PAUSE_() YieldProcessor()
#elif defined(__GNUC__)
#if defined(__i386__) || defined(__x86_64__)
/*
    See: Intel(R) 64 and IA-32 Architectures Software Developer's Manual V2
         PAUSE-Spin Loop Hint, 4-57
         http://www.intel.com/content/www/us/en/architecture-and-technology/64-ia-32-architectures-software-developer-instruction-set-reference-manual-325383.html?wapkw=instruction+set+reference
*/
#   define IPC_LOCK_PAUSE_() __asm__ __volatile__("pause")
#elif defined(__ia64__) || defined(__ia64)
/*
    See: Intel(R) Itanium(R) Architecture Developer's Manual, Vol.3
         hint - Performance Hint, 3:145
         http://www.intel.com/content/www/us/en/processors/itanium/itanium-architecture-vol-3-manual.html
*/
#   define IPC_LOCK_PAUSE_() __asm__ __volatile__ ("hint @pause")
#elif defined(__arm__)
/*
    See: ARM Architecture Reference Manuals (YIELD)
         http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.subset.architecture.reference/index.html
*/
#   define IPC_LOCK_PAUSE_() __asm__ __volatile__ ("yield")
#endif
#endif/*compilers*/

#if !defined(IPC_LOCK_PAUSE_)
/*
    Just use a compiler fence, prevent compiler from optimizing loop
*/
#   define IPC_LOCK_PAUSE_() std::atomic_signal_fence(std::memory_order_seq_cst)
#endif/*!defined(IPC_LOCK_PAUSE_)*/

////////////////////////////////////////////////////////////////
/// Yield to other threads
////////////////////////////////////////////////////////////////

namespace ipc {

template <typename K>
inline void yield(K& k) noexcept {
    if (k < 4)  { /* Do nothing */ }
    else
    if (k < 16) { IPC_LOCK_PAUSE_(); }
    else
    if (k < 32) { std::this_thread::yield(); }
    else {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return;
    }
    ++k;
}

template <std::size_t N = 32, typename K, typename F>
inline void sleep(K& k, F&& f) {
    if (k < static_cast<K>(N)) {
        std::this_thread::yield();
    }
    else {
        static_cast<void>(std::forward<F>(f)());
        return;
    }
    ++k;
}

template <std::size_t N = 32, typename K>
inline void sleep(K& k) {
    sleep<N>(k, [] {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    });
}

} // namespace ipc

#pragma pop_macro("IPC_LOCK_PAUSE_")

namespace ipc {

class spin_lock {
    std::atomic<unsigned> lc_ { 0 };

public:
    void lock(void) noexcept {
        for (unsigned k = 0;
             lc_.exchange(1, std::memory_order_acquire);
             yield(k)) ;
    }

    void unlock(void) noexcept {
        lc_.store(0, std::memory_order_release);
    }
};

class rw_lock {
    using lc_ui_t = unsigned;

    std::atomic<lc_ui_t> lc_ { 0 };

    enum : lc_ui_t {
        w_mask = (std::numeric_limits<std::make_signed_t<lc_ui_t>>::max)(), // b 0111 1111
        w_flag = w_mask + 1                                                 // b 1000 0000
    };

public:
    rw_lock() = default;

    rw_lock(const rw_lock&) = delete;
    rw_lock& operator=(const rw_lock&) = delete;
    rw_lock(rw_lock&&) = delete;
    rw_lock& operator=(rw_lock&&) = delete;

    void lock() noexcept {
        for (unsigned k = 0;;) {
            auto old = lc_.fetch_or(w_flag, std::memory_order_acq_rel);
            if (!old) return;           // got w-lock
            if (!(old & w_flag)) break; // other thread having r-lock
            yield(k);                   // other thread having w-lock
        }
        // wait for reading finished
        for (unsigned k = 0;
             lc_.load(std::memory_order_acquire) & w_mask;
             yield(k)) ;
    }

    void unlock() noexcept {
        lc_.store(0, std::memory_order_release);
    }

    void lock_shared() noexcept {
        auto old = lc_.load(std::memory_order_acquire);
        for (unsigned k = 0;;) {
            // if w_flag set, just continue
            if (old & w_flag) {
                yield(k);
                old = lc_.load(std::memory_order_acquire);
            }
            // otherwise try cas lc + 1 (set r-lock)
            else if (lc_.compare_exchange_weak(old, old + 1, std::memory_order_release)) {
                return;
            }
            // set r-lock failed, old has been updated
        }
    }

    void unlock_shared() noexcept {
        lc_.fetch_sub(1, std::memory_order_release);
    }
};

} // namespace ipc
