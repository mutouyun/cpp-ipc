#pragma once

#include <atomic>
#include <limits>
#include <type_traits>

#include "def.h"

namespace ipc {

class rw_lock {
    using lc_ui_t = std::size_t;
    std::atomic<lc_ui_t> lc_ { 0 };

    enum : lc_ui_t {
        w_mask = (std::numeric_limits<std::make_signed_t<lc_ui_t>>::max)(), // b 0111 1111
        w_flag = w_mask + 1                                                 // b 1000 0000
    };

public:
    void lock() {
        for (unsigned k = 0;; ++k) {
            auto old = lc_.fetch_or(w_flag, std::memory_order_acquire);
            if (!old) return;           // got w-lock
            if (!(old & w_flag)) break; // other thread having r-lock
            yield(k);                   // other thread having w-lock
        }
        // wait for reading finished
        for (unsigned k = 0; lc_.load(std::memory_order_acquire) & w_mask; ++k) {
            yield(k);
        }
    }

    void unlock() {
        lc_.store(0, std::memory_order_release);
    }

    void lock_shared() {
        for (unsigned k = 0;; ++k) {
            auto old = lc_.load(std::memory_order_relaxed);
            // if w_flag set, just continue; otherwise cas lc + 1 (set r-lock)
            if (!(old & w_flag) &&
                lc_.compare_exchange_weak(old, old + 1, std::memory_order_acquire)) {
                break;
            }
            yield(k);
            std::atomic_thread_fence(std::memory_order_acquire);
        }
    }

    void unlock_shared() {
        lc_.fetch_sub(1, std::memory_order_release);
    }
};

} // namespace ipc
