#pragma once

#include <Windows.h>

#include <algorithm>
#include <iterator>
#include <atomic>
#include <array>
#include <tuple>

#include "rw_lock.h"
#include "pool_alloc.h"

#include "platform/to_tchar.h"
#include "platform/detail.h"

namespace ipc {
namespace detail {

class waiter {
    long volatile counter_ = 0;
    spin_lock     lock_;

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
        if (h == invalid()) return;
        ::CloseHandle(h);
    }

    template <typename F>
    bool wait_if(handle_t h, F&& pred) {
        if (h == invalid()) return false;
        {
            IPC_UNUSED_ auto guard = ipc::detail::unique_lock(lock_);
            if (!std::forward<F>(pred)()) return true;
            ++ counter_;
        }
        return ::WaitForSingleObject(h, INFINITE) == WAIT_OBJECT_0;
    }

    void notify(handle_t h) {
        if (h == invalid()) return;
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(lock_);
        if (counter_ == 0) return;
        -- counter_;
        ::ReleaseSemaphore(h, 1, NULL);
    }

    void broadcast(handle_t h) {
        if (h == invalid()) return;
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(lock_);
        if (counter_ == 0) return;
        long all_count = counter_;
        counter_ = 0;
        ::ReleaseSemaphore(h, all_count, NULL);
    }
};

} // namespace detail
} // namespace ipc
