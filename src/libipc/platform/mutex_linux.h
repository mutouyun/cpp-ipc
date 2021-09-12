#pragma once

#include <cstring>
#include <cassert>
#include <system_error>
#include <mutex>

#include <pthread.h>

#include "libipc/platform/get_wait_time.h"
#include "libipc/platform/detail.h"
#include "libipc/utility/log.h"
#include "libipc/utility/scope_guard.h"
#include "libipc/memory/resource.h"
#include "libipc/shm.h"

namespace ipc {
namespace detail {
namespace sync {

class mutex {
    ipc::shm::handle shm_;
    pthread_mutex_t *mutex_ = nullptr;

    pthread_mutex_t *acquire_mutex(char const *name) {
        if (!shm_.acquire(name, sizeof(pthread_mutex_t))) {
            ipc::error("[acquire_mutex] fail shm.acquire: %s\n", name);
            return nullptr;
        }
        return static_cast<pthread_mutex_t *>(shm_.get());
    }

    pthread_mutex_t *get_mutex(char const *name) {
        if (name == nullptr) {
            return nullptr;
        }
        static ipc::map<ipc::string, pthread_mutex_t *> mutex_handles;
        static std::mutex lock;
        IPC_UNUSED_ std::lock_guard<std::mutex> guard {lock};
        auto it = mutex_handles.find(name);
        if (it == mutex_handles.end()) {
            auto ptr = acquire_mutex(name);
            if (ptr != nullptr) {
                mutex_handles.emplace(name, ptr);
            }
            return ptr;
        }
        return it->second;
    }

public:
    mutex() = default;
    ~mutex() = default;

    pthread_mutex_t const *native() const noexcept {
        return mutex_;
    }

    pthread_mutex_t *native() noexcept {
        return mutex_;
    }

    bool valid() const noexcept {
        static const char tmp[sizeof(pthread_mutex_t)] {};
        return (mutex_ != nullptr)
            && (std::memcmp(tmp, mutex_, sizeof(pthread_mutex_t)) != 0);
    }

    bool open(char const *name) noexcept {
        close();
        if ((mutex_ = get_mutex(name)) == nullptr) {
            return false;
        }
        if (shm_.ref() == 1) {
            ::pthread_mutex_destroy(mutex_);
            auto finally = ipc::guard([this] { close(); }); // close when failed
            // init mutex
            int eno;
            pthread_mutexattr_t mutex_attr;
            if ((eno = ::pthread_mutexattr_init(&mutex_attr)) != 0) {
                ipc::error("fail pthread_mutexattr_init[%d]\n", eno);
                return false;
            }
            IPC_UNUSED_ auto guard_mutex_attr = unique_ptr(&mutex_attr, ::pthread_mutexattr_destroy);
            if ((eno = ::pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED)) != 0) {
                ipc::error("fail pthread_mutexattr_setpshared[%d]\n", eno);
                return false;
            }
            if ((eno = ::pthread_mutexattr_setrobust(&mutex_attr, PTHREAD_MUTEX_ROBUST)) != 0) {
                ipc::error("fail pthread_mutexattr_setrobust[%d]\n", eno);
                return false;
            }
            *mutex_ = PTHREAD_MUTEX_INITIALIZER;
            if ((eno = ::pthread_mutex_init(mutex_, &mutex_attr)) != 0) {
                ipc::error("fail pthread_mutex_init[%d]\n", eno);
                return false;
            }
            finally.dismiss();
        }
        return valid();
    }

    void close() noexcept {
        if (shm_.ref() == 1) {
            int eno;
            if ((eno = ::pthread_mutex_destroy(mutex_)) != 0) {
                ipc::error("fail pthread_mutex_destroy[%d]\n", eno);
            }
        }
        shm_.release();
        mutex_ = nullptr;
    }

    bool lock(std::uint64_t tm) noexcept {
        for (;;) {
            auto ts = detail::make_timespec(tm);
            int eno = (tm == invalid_value) 
                ? ::pthread_mutex_lock(mutex_) 
                : ::pthread_mutex_timedlock(mutex_, &ts);
            switch (eno) {
            case 0:
                return true;
            case ETIMEDOUT:
                return false;
            case EOWNERDEAD: {
                    if (shm_.ref() > 1) {
                        shm_.sub_ref();
                    }
                    int eno2 = ::pthread_mutex_consistent(mutex_);
                    if (eno2 != 0) {
                        ipc::error("fail pthread_mutex_lock[%d], pthread_mutex_consistent[%d]\n", eno, eno2);
                        return false;
                    }
                    int eno3 = ::pthread_mutex_unlock(mutex_);
                    if (eno3 != 0) {
                        ipc::error("fail pthread_mutex_lock[%d], pthread_mutex_unlock[%d]\n", eno, eno3);
                        return false;
                    }
                }
                break; // loop again
            default:
                ipc::error("fail pthread_mutex_lock[%d]\n", eno);
                return false;
            }
        }
    }

    bool try_lock() noexcept(false) {
        auto ts = detail::make_timespec(0);
        int eno = ::pthread_mutex_timedlock(mutex_, &ts);
        switch (eno) {
        case 0:
            return true;
        case ETIMEDOUT:
            return false;
        case EOWNERDEAD: {
                if (shm_.ref() > 1) {
                    shm_.sub_ref();
                }
                int eno2 = ::pthread_mutex_consistent(mutex_);
                if (eno2 != 0) {
                    ipc::error("fail pthread_mutex_timedlock[%d], pthread_mutex_consistent[%d]\n", eno, eno2);
                    break;
                }
                int eno3 = ::pthread_mutex_unlock(mutex_);
                if (eno3 != 0) {
                    ipc::error("fail pthread_mutex_timedlock[%d], pthread_mutex_unlock[%d]\n", eno, eno3);
                    break;
                }
            }
            break;
        default:
            ipc::error("fail pthread_mutex_timedlock[%d]\n", eno);
            break;
        }
        throw std::system_error{eno, std::system_category()};
    }

    bool unlock() noexcept {
        int eno;
        if ((eno = ::pthread_mutex_unlock(mutex_)) != 0) {
            ipc::error("fail pthread_mutex_unlock[%d]\n", eno);
            return false;
        }
        return true;
    }
};

} // namespace sync
} // namespace detail
} // namespace ipc
