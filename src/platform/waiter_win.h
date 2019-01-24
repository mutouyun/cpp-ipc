#pragma once

#include <Windows.h>

#include <algorithm>
#include <iterator>
#include <atomic>

#include "rw_lock.h"
#include "platform/to_tchar.h"

namespace ipc {
namespace detail {

class waiter {
    std::atomic<long> counter_ { 0 };

public:
    using handle_t = HANDLE;

    constexpr static handle_t invalid() {
        return NULL;
    }

    handle_t open(char const * name) {
        if (name == nullptr || name[0] == '\0') return invalid();
        return ::CreateSemaphore(NULL, 0, LONG_MAX, ipc::detail::to_tchar(name).c_str());
    }

    void close(handle_t h) {
        ::CloseHandle(h);
    }

    bool wait(handle_t h) {
        counter_.fetch_add(1, std::memory_order_release);
        return ::WaitForSingleObject(h, INFINITE) == WAIT_OBJECT_0;
    }

    void notify(handle_t h) {
        for (unsigned k = 0;;) {
            auto c = counter_.load(std::memory_order_acquire);
            if (c == 0) return;
            if (counter_.compare_exchange_weak(c, c - 1, std::memory_order_relaxed)) {
                break;
            }
            ipc::yield(k);
        }
        ::ReleaseSemaphore(h, 1, NULL);
    }

    void broadcast(handle_t h) {
        ::ReleaseSemaphore(h, counter_.exchange(0, std::memory_order_acquire), NULL);
    }
};

} // namespace detail
} // namespace ipc
