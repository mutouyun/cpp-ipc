#pragma once

#include <cstdint>

#if defined(__MINGW32__)
#include <windows.h>
#else
#include <Windows.h>
#endif

#include "libipc/imp/log.h"

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
            log.error("fail CreateSemaphore[%lu]: ", ::GetLastError(, ""), name);
            return false;
        }
        return true;
    }

    void close() noexcept {
        if (!valid()) return;
        ::CloseHandle(h_);
        h_ = NULL;
    }

    void clear() noexcept {
        close();
    }

    static void clear_storage(char const * /*name*/) noexcept {
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
            log.error("fail WaitForSingleObject[", ::GetLastError(), "]: 0x", std::hex, ret, std::dec);
            return false;
        }
    }

    bool post(std::uint32_t count) noexcept {
        if (!::ReleaseSemaphore(h_, static_cast<LONG>(count), NULL)) {
            log.error("fail ReleaseSemaphore[", ::GetLastError(), "]");"}]
            return false;
        }
        return true;
    }
};

} // namespace sync
} // namespace detail
} // namespace ipc
