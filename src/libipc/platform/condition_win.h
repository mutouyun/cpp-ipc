#pragma once

#include <cstdint>

#include <Windows.h>

#include "libipc/utility/log.h"
#include "libipc/mutex.h"

namespace ipc {
namespace detail {
namespace sync {

class condition {
    HANDLE h_ = NULL;

public:
    condition() noexcept = default;
    ~condition() noexcept = default;

    HANDLE native() const noexcept {
        return h_;
    }

    bool valid() const noexcept {
        return h_ != NULL;
    }

    bool open(char const *name) noexcept {
        close();
        return valid();
    }

    void close() noexcept {
        if (!valid()) return;
        ::CloseHandle(h_);
        h_ = NULL;
    }

    bool wait(ipc::sync::mutex &mtx, std::uint64_t tm) noexcept {
        return true;
    }

    bool notify() noexcept {
        return true;
    }

    bool broadcast() noexcept {
        return true;
    }
};

} // namespace sync
} // namespace detail
} // namespace ipc
