#pragma once

#include <cstdint>

#include <fcntl.h>      /* For O_* constants */
#include <sys/stat.h>   /* For mode constants */
#include <semaphore.h>
#include <errno.h>

#include "libipc/utility/log.h"
#include "libipc/shm.h"

#include "get_wait_time.h"

namespace ipc {
namespace detail {
namespace sync {

class semaphore {
    ipc::shm::handle shm_;
    sem_t *h_ = SEM_FAILED;

public:
    semaphore() = default;
    ~semaphore() noexcept = default;

    sem_t *native() const noexcept {
        return h_;
    }

    bool valid() const noexcept {
        return h_ != SEM_FAILED;
    }

    bool open(char const *name, std::uint32_t count) noexcept {
        close();
        if (!shm_.acquire(name, 1)) {
            ipc::error("[open_semaphore] fail shm.acquire: %s\n", name);
            return false;
        }
        h_ = ::sem_open(name, O_CREAT, 0666, static_cast<unsigned>(count));
        if (h_ == SEM_FAILED) {
            ipc::error("fail sem_open[%d]: %s\n", errno, name);
            return false;
        }
        return true;
    }

    void close() noexcept {
        if (!valid()) return;
        if (::sem_close(h_) != 0) {
            ipc::error("fail sem_close[%d]: %s\n", errno);
        }
        h_ = SEM_FAILED;
        if (shm_.name() != nullptr) {
            std::string name = shm_.name();
            if (shm_.release() <= 1) {
                if (::sem_unlink(name.c_str()) != 0) {
                    ipc::error("fail sem_unlink[%d]: %s, name: %s\n", errno, name.c_str());
                }
            }
        }
    }

    bool wait(std::uint64_t tm) noexcept {
        if (!valid()) return false;
        if (tm == invalid_value) {
            if (::sem_wait(h_) != 0) {
                ipc::error("fail sem_wait[%d]: %s\n", errno);
                return false;
            }
        } else {
            auto ts = detail::make_timespec(tm);
            if (::sem_timedwait(h_, &ts) != 0) {
                if (errno != ETIMEDOUT) {
                    ipc::error("fail sem_timedwait[%d]: tm = %zd, tv_sec = %ld, tv_nsec = %ld\n",
                                errno, tm, ts.tv_sec, ts.tv_nsec);
                }
                return false;
            }
        }
        return true;
    }

    bool post(std::uint32_t count) noexcept {
        if (!valid()) return false;
        for (std::uint32_t i = 0; i < count; ++i) {
            if (::sem_post(h_) != 0) {
                ipc::error("fail sem_post[%d]: %s\n", errno);
                return false;
            }
        }
        return true;
    }
};

} // namespace sync
} // namespace detail
} // namespace ipc
