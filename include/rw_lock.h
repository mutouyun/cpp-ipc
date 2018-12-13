#pragma once

#include <atomic>
#include <limits>
#include <type_traits>

#include "def.h"

namespace ipc {

class rw_cas_lock {
    std::atomic_size_t lc_ { 0 };

    enum : std::size_t {
        w_flag = (std::numeric_limits<std::size_t>::max)()
    };

public:
    void lock() {
        for (unsigned k = 0;; ++k) {
            std::size_t expected = 0;
            if (lc_.compare_exchange_weak(expected, w_flag, std::memory_order_acquire)) {
                break;
            }
            yield(k);
        }
    }

    void unlock() {
        lc_.store(0, std::memory_order_release);
    }

    void lock_shared() {
        for (unsigned k = 0;; ++k) {
            std::size_t old = lc_.load(std::memory_order_relaxed);
            std::size_t unlocked = old + 1;
            if (unlocked &&
                lc_.compare_exchange_weak(old, unlocked, std::memory_order_acquire)) {
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

class rw_lock {
    using lc_ui_t = std::size_t;
    std::atomic<lc_ui_t> lc_ { 0 };

    enum : lc_ui_t {
        w_mask = (std::numeric_limits<std::make_signed_t<lc_ui_t>>::max)(), // b 0111 1111
        w_flag = w_mask + 1                                                 // b 1000 0000
    };

public:
    void lock() {
        auto old = lc_.fetch_or(w_flag, std::memory_order_acquire);
        if (!old) return;
        // just like a spin-lock
        if (old & w_flag) for (unsigned k = 1; lc_.fetch_or(w_flag, std::memory_order_acquire) & w_flag; ++k) {
            yield(k);
        }
        // wait for reading finished
        else for (unsigned k = 1; lc_.load(std::memory_order_acquire) & w_mask; ++k) {
            yield(k);
        }
    }

    void unlock() {
        lc_.fetch_and(w_mask, std::memory_order_release);
    }

    void lock_shared() {
        for (unsigned k = 0;; ++k) {
            auto old = lc_.load(std::memory_order_relaxed);
            // if w_flag set, just continue; otherwise cas ++
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
