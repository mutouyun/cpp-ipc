#pragma once

#include <cstdint>
#include <cstring>

#include <pthread.h>

#include "libipc/utility/log.h"
#include "libipc/utility/scope_guard.h"
#include "libipc/mutex.h"
#include "libipc/shm.h"

#include "get_wait_time.h"

namespace ipc {
namespace detail {
namespace sync {

class condition {
    ipc::shm::handle shm_;
    pthread_cond_t *cond_ = nullptr;

    pthread_cond_t *acquire_cond(char const *name) {
        if (!shm_.acquire(name, sizeof(pthread_cond_t))) {
            ipc::error("[acquire_cond] fail shm.acquire: %s\n", name);
            return nullptr;
        }
        return static_cast<pthread_cond_t *>(shm_.get());
    }

public:
    condition() = default;
    ~condition() = default;

    pthread_cond_t const *native() const noexcept {
        return cond_;
    }

    pthread_cond_t *native() noexcept {
        return cond_;
    }

    bool valid() const noexcept {
        static const char tmp[sizeof(pthread_cond_t)] {};
        return (cond_ != nullptr)
            && (std::memcmp(tmp, cond_, sizeof(pthread_cond_t)) != 0);
    }

    bool open(char const *name) noexcept {
        close();
        if ((cond_ = acquire_cond(name)) == nullptr) {
            return false;
        }
        if (shm_.ref() > 1) {
            return valid();
        }
        ::pthread_cond_destroy(cond_);
        auto finally = ipc::guard([this] { close(); }); // close when failed
        // init condition
        int eno;
        pthread_condattr_t cond_attr;
        if ((eno = ::pthread_condattr_init(&cond_attr)) != 0) {
            ipc::error("fail pthread_condattr_init[%d]\n", eno);
            return false;
        }
        IPC_UNUSED_ auto guard_cond_attr = guard([&cond_attr] { ::pthread_condattr_destroy(&cond_attr); });
        if ((eno = ::pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED)) != 0) {
            ipc::error("fail pthread_condattr_setpshared[%d]\n", eno);
            return false;
        }
        *cond_ = PTHREAD_COND_INITIALIZER;
        if ((eno = ::pthread_cond_init(cond_, &cond_attr)) != 0) {
            ipc::error("fail pthread_cond_init[%d]\n", eno);
            return false;
        }
        finally.dismiss();
        return valid();
    }

    void close() noexcept {
        if ((shm_.ref() <= 1) && cond_ != nullptr) {
            int eno;
            if ((eno = ::pthread_cond_destroy(cond_)) != 0) {
                ipc::error("fail pthread_cond_destroy[%d]\n", eno);
            }
        }
        shm_.release();
        cond_ = nullptr;
    }

    bool wait(ipc::sync::mutex &mtx, std::uint64_t tm) noexcept {
        if (!valid()) return false;
        switch (tm) {
        case invalid_value: {
                int eno;
                if ((eno = ::pthread_cond_wait(cond_, static_cast<pthread_mutex_t *>(mtx.native()))) != 0) {
                    ipc::error("fail pthread_cond_wait[%d]\n", eno);
                    return false;
                }
            }
            break;
        default: {
                auto ts = detail::make_timespec(tm);
                int eno;
                if ((eno = ::pthread_cond_timedwait(cond_, static_cast<pthread_mutex_t *>(mtx.native()), &ts)) != 0) {
                    if (eno != ETIMEDOUT) {
                        ipc::error("fail pthread_cond_timedwait[%d]: tm = %zd, tv_sec = %ld, tv_nsec = %ld\n",
                                   eno, tm, ts.tv_sec, ts.tv_nsec);
                    }
                    return false;
                }
            }
            break;
        }
        return true;
    }

    bool notify(ipc::sync::mutex &) noexcept {
        if (!valid()) return false;
        int eno;
        if ((eno = ::pthread_cond_signal(cond_)) != 0) {
            ipc::error("fail pthread_cond_signal[%d]\n", eno);
            return false;
        }
        return true;
    }

    bool broadcast(ipc::sync::mutex &) noexcept {
        if (!valid()) return false;
        int eno;
        if ((eno = ::pthread_cond_broadcast(cond_)) != 0) {
            ipc::error("fail pthread_cond_broadcast[%d]\n", eno);
            return false;
        }
        return true;
    }
};

} // namespace sync
} // namespace detail
} // namespace ipc
