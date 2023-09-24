#pragma once

#include <cstdint>

#include <Windows.h>

#include "libipc/utility/log.h"

#include "to_tchar.h"
#include "get_sa.h"

namespace ipc {
namespace detail {
namespace sync {

class semaphore {
    HANDLE h_ = NULL;

public:
    semaphore() noexcept = default;
    ~semaphore() noexcept = default;

    HANDLE native() const noexcept {
        return h_;
    }

    bool valid() const noexcept {
        return h_ != NULL;
    }

    bool open(char const *name, std::uint32_t count) noexcept {
        close();
        h_ = ::CreateSemaphore(detail::get_sa(), 
                               static_cast<LONG>(count), LONG_MAX, 
                               detail::to_tchar(name).c_str());
        if (h_ == NULL) {
            ipc::error("fail CreateSemaphore[%lu]: %s\n", ::GetLastError(), name);
            return false;
        }
        return true;
    }

    void close() noexcept {
        if (!valid()) return;
        ::CloseHandle(h_);
        h_ = NULL;
    }

    bool wait(std::uint64_t tm) noexcept {
        DWORD ret, ms = (tm == invalid_value) ? INFINITE : static_cast<DWORD>(tm);
        switch ((ret = ::WaitForSingleObject(h_, ms))) {
        case WAIT_OBJECT_0:
            return true;
        case WAIT_TIMEOUT:
            return false;
        case WAIT_ABANDONED:
        default:
            ipc::error("fail WaitForSingleObject[%lu]: 0x%08X\n", ::GetLastError(), ret);
            return false;
        }
    }

    bool post(std::uint32_t count) noexcept {
        if (!::ReleaseSemaphore(h_, static_cast<LONG>(count), NULL)) {
            ipc::error("fail ReleaseSemaphore[%lu]\n", ::GetLastError());
            return false;
        }
        return true;
    }
};

} // namespace sync
} // namespace detail
} // namespace ipc
