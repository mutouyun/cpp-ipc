#pragma once

#include <utility>
#include <string>
#include <mutex>

#include "libipc/def.h"
#include "libipc/mutex.h"
#include "libipc/condition.h"
#include "libipc/platform/detail.h"

namespace ipc {
namespace detail {

class waiter {
    ipc::sync::condition cond_;
    ipc::sync::mutex     lock_;

public:
    waiter() = default;
    waiter(char const *name) {
        open(name);
    }

    ~waiter() {
        close();
    }

    bool valid() const noexcept {
        return cond_.valid() && lock_.valid();
    }

    bool open(char const *name) noexcept {
        if (!cond_.open((std::string{"_waiter_cond_"} + name).c_str())) {
            return false;
        }
        if (!lock_.open((std::string{"_waiter_lock_"} + name).c_str())) {
            cond_.close();
            return false;
        }
        return valid();
    }

    void close() noexcept {
        cond_.close();
        lock_.close();
    }

    template <typename F>
    bool wait_if(F &&pred, std::uint64_t tm = ipc::invalid_value) noexcept {
        IPC_UNUSED_ std::lock_guard<ipc::sync::mutex> guard {lock_};
        while (std::forward<F>(pred)()) {
            if (!cond_.wait(lock_, tm)) return false;
        }
        return true;
    }

    bool notify() noexcept {
        std::lock_guard<ipc::sync::mutex>{lock_}; // barrier
        return cond_.notify();
    }

    bool broadcast() noexcept {
        std::lock_guard<ipc::sync::mutex>{lock_}; // barrier
        return cond_.broadcast();
    }
};

} // namespace detail
} // namespace ipc
