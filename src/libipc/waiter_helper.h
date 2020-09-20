#pragma once

#include <atomic>
#include <limits>
#include <utility>

#include "libipc/def.h"
#include "libipc/utility/scope_guard.h"

namespace ipc {
namespace detail {

class waiter_helper {

    enum : unsigned {
        destruct_mask = (std::numeric_limits<unsigned>::max)() >> 1,
        destruct_flag = ~destruct_mask
    };

public:
    struct wait_counter {
        std::atomic<unsigned> waiting_ { 0 };
        long counter_ = 0;
    };

    struct wait_flags {
        std::atomic<bool> is_waiting_ { false };
        std::atomic<bool> is_closed_  { true  };
    };

    template <typename Mutex, typename Ctrl, typename F>
    static bool wait_if(Ctrl & ctrl, Mutex & mtx, F && pred, std::size_t tm) {
        auto & flags = ctrl.flags();
        if (flags.is_closed_.load(std::memory_order_acquire)) {
            return false;
        }

        auto & counter = ctrl.counter();
        counter.waiting_.fetch_add(1, std::memory_order_release);
        flags.is_waiting_.store(true, std::memory_order_relaxed);
        auto finally = ipc::guard([&counter, &flags] {
            counter.waiting_.fetch_sub(1, std::memory_order_release);
            flags.is_waiting_.store(false, std::memory_order_relaxed);
        });
        {
            IPC_UNUSED_ auto guard = ctrl.get_lock();
            if (!std::forward<F>(pred)()) return true;
            counter.counter_ += 1;
        }
        mtx.unlock();

        bool ret = false;
        do {
            bool is_waiting = flags.is_waiting_.load(std::memory_order_relaxed);
            bool is_closed  = flags.is_closed_ .load(std::memory_order_acquire);
            if (!is_waiting || is_closed) {
                ret = false;
                break;
            }
            ret = ctrl.sema_wait(tm);
        } while (counter.waiting_.load(std::memory_order_acquire) & destruct_flag);
        finally.do_exit();
        ret = ctrl.handshake_post(1) && ret;

        mtx.lock();
        return ret;
    }

    template <typename Ctrl>
    static bool notify(Ctrl & ctrl) {
        auto & counter = ctrl.counter();
        if ((counter.waiting_.load(std::memory_order_acquire) & destruct_mask) == 0) {
            return true;
        }
        bool ret = true;
        IPC_UNUSED_ auto guard = ctrl.get_lock();
        if (counter.counter_ > 0) {
            ret = ctrl.sema_post(1);
            counter.counter_ -= 1;
            ret = ret && ctrl.handshake_wait(default_timeout);
        }
        return ret;
    }

    template <typename Ctrl>
    static bool broadcast(Ctrl & ctrl) {
        auto & counter = ctrl.counter();
        if ((counter.waiting_.load(std::memory_order_acquire) & destruct_mask) == 0) {
            return true;
        }
        bool ret = true;
        IPC_UNUSED_ auto guard = ctrl.get_lock();
        if (counter.counter_ > 0) {
            ret = ctrl.sema_post(counter.counter_);
            do {
                counter.counter_ -= 1;
                ret = ret && ctrl.handshake_wait(default_timeout);
            } while (counter.counter_ > 0);
        }
        return ret;
    }

    template <typename Ctrl>
    static bool quit_waiting(Ctrl & ctrl) {
        auto & flags = ctrl.flags();
        if (!flags.is_waiting_.exchange(false, std::memory_order_release)) {
            return true;
        }
        auto & counter = ctrl.counter();
        if ((counter.waiting_.load(std::memory_order_acquire) & destruct_mask) == 0) {
            return true;
        }
        bool ret = true;
        IPC_UNUSED_ auto guard = ctrl.get_lock();
        counter.waiting_.fetch_or(destruct_flag, std::memory_order_relaxed);
        IPC_UNUSED_ auto finally = ipc::guard([&counter] {
            counter.waiting_.fetch_and(destruct_mask, std::memory_order_relaxed);
        });
        if (counter.counter_ > 0) {
            ret = ctrl.sema_post(counter.counter_);
            counter.counter_ -= 1;
            ret = ret && ctrl.handshake_wait(default_timeout);
        }
        return ret;
    }
};

} // namespace detail
} // namespace ipc
