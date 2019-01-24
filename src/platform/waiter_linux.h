#pragma once

#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

#include <cstring>
#include <atomic>

#include "def.h"
#include "rw_lock.h"

#include "platform/detail.h"

namespace ipc {
namespace detail {

class waiter {
    std::atomic<unsigned> rc_      { 0 };
    std::atomic<unsigned> counter_ { 0 };

    spin_lock lc_;

public:
    using handle_t = sem_t*;

private:
    bool post(handle_t h) {
        for (unsigned k = 0;;) {
            auto c = counter_.load(std::memory_order_acquire);
            if (c == 0) return false;
            if (counter_.compare_exchange_weak(c, c - 1, std::memory_order_relaxed)) {
                break;
            }
            ipc::yield(k);
        }
        return ::sem_post(h) == 0;
    }

public:
    constexpr static handle_t invalid() {
        return SEM_FAILED;
    }

    handle_t open(char const * name) {
        if (name == nullptr || name[0] == '\0') return invalid();
        rc_.fetch_add(1, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_release);
        return ::sem_open(name, O_CREAT | O_RDWR,
                                S_IRUSR | S_IWUSR |
                                S_IRGRP | S_IWGRP |
                                S_IROTH | S_IWOTH, 0);
    }

    void close(handle_t h, char const * name) {
        if (h == invalid()) return;
        if (name == nullptr || name[0] == '\0') return;
        ::sem_close(h);
        if (rc_.fetch_sub(1, std::memory_order_acquire) == 1) {
            ::sem_unlink(name);
        }
    }

    bool wait(handle_t h) {
        if (h == invalid()) return false;
        {
            IPC_UNUSED_ auto guard = ipc::detail::unique_lock(lc_);
            counter_.fetch_add(1, std::memory_order_relaxed);
        }
        bool ret = (::sem_wait(h) == 0);
        return ret;
    }

    void notify(handle_t h) {
        if (h == invalid()) return;
        post(h);
    }

    void broadcast(handle_t h) {
        if (h == invalid()) return;
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(lc_);
        while (post(h)) ;
    }
};

} // namespace detail
} // namespace ipc
