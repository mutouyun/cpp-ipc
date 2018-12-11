#pragma once

#include <atomic>
#include <thread>
#include <limits>

namespace ipc {

class rw_lock {
    std::atomic_size_t lc_ { 0 };

    enum : std::size_t {
        w_flag = std::numeric_limits<std::size_t>::max()
    };

public:
    void r_lock(void) {
        while(1) {
            std::size_t old = lc_.load(std::memory_order_relaxed);
            std::size_t unlocked = old + 1;
            if (unlocked &&
                lc_.compare_exchange_weak(old, unlocked, std::memory_order_acq_rel)) {
                break;
            }
            std::this_thread::yield();
        }
    }

    void r_unlock(void) {
        lc_.fetch_sub(1, std::memory_order_release);
    }

    void w_lock(void) {
        while (1) {
            std::size_t expected = 0;
            if (lc_.compare_exchange_weak(expected, w_flag, std::memory_order_acq_rel)) {
                break;
            }
            std::this_thread::yield();
        }
    }

    void w_unlock(void) {
        lc_.store(0, std::memory_order_release);
    }
};

} // namespace ipc
