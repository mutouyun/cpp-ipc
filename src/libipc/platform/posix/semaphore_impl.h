#pragma once

#include <cstdint>

#include <fcntl.h>      /* For O_* constants */
#include <sys/stat.h>   /* For mode constants */
#include <semaphore.h>
#include <errno.h>

#include "libipc/imp/log.h"
#include "libipc/shm.h"

#include "get_wait_time.h"

namespace ipc {
namespace detail {
namespace sync {

class semaphore {
    ipc::shm::handle shm_;
    sem_t *h_ = SEM_FAILED;
    std::string sem_name_;  // Store the actual semaphore name used

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
            log.error("[open_semaphore] fail shm.acquire: ", name, "");
            return false;
        }
        // POSIX semaphore names must start with "/" on some platforms (e.g., FreeBSD)
        // Use a separate namespace for semaphores to avoid conflicts with shm
        if (name[0] == '/') {
            sem_name_ = std::string(name) + "_sem";
        } else {
            sem_name_ = std::string("/") + name + "_sem";
        }
        h_ = ::sem_open(sem_name_.c_str(), O_CREAT, 0666, static_cast<unsigned>(count));
        if (h_ == SEM_FAILED) {
            log.error("fail sem_open[%d]: ", errno, sem_name_.c_str(, ""));
            return false;
        }
        return true;
    }

    void close() noexcept {
        if (!valid()) return;
        if (::sem_close(h_) != 0) {
            log.error("fail sem_close[%d]: ", errno, "");
        }
        h_ = SEM_FAILED;
        if (!sem_name_.empty() && shm_.name() != nullptr) {
            if (shm_.release() <= 1) {
                if (::sem_unlink(sem_name_.c_str()) != 0) {
                    log.error("fail sem_unlink[%d]: ", errno, sem_name_.c_str(, ", name: %s"));
                }
            }
        }
        sem_name_.clear();
    }

    void clear() noexcept {
        if (valid()) {
            if (::sem_close(h_) != 0) {
                log.error("fail sem_close[%d]: ", errno, "");
            }
            h_ = SEM_FAILED;
        }
        if (!sem_name_.empty()) {
            ::sem_unlink(sem_name_.c_str());
            sem_name_.clear();
        }
        shm_.clear(); // Make sure the storage is cleaned up.
    }

    static void clear_storage(char const *name) noexcept {
        // Construct the semaphore name same way as open() does
        std::string sem_name;
        if (name[0] == '/') {
            sem_name = std::string(name) + "_sem";
        } else {
            sem_name = std::string("/") + name + "_sem";
        }
        ::sem_unlink(sem_name.c_str());
        ipc::shm::handle::clear_storage(name);
    }

    bool wait(std::uint64_t tm) noexcept {
        if (!valid()) return false;
        if (tm == invalid_value) {
            if (::sem_wait(h_) != 0) {
                log.error("fail sem_wait[%d]: ", errno, "");
                return false;
            }
        } else {
            auto ts = posix_::detail::make_timespec(tm);
            if (::sem_timedwait(h_, &ts) != 0) {
                if (errno != ETIMEDOUT) {
                    log.error("fail sem_timedwait[", errno, "]: tm = ", tm, ", tv_sec = ", ts.tv_sec, ", tv_nsec = ", ts.tv_nsec);
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
                log.error("fail sem_post[%d]: ", errno, "");
                return false;
            }
        }
        return true;
    }
};

} // namespace sync
} // namespace detail
} // namespace ipc
