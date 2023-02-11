#pragma once

#include <cstring>
#include <cassert>
#include <cstdint>
#include <system_error>
#include <mutex>
#include <atomic>

#include <pthread.h>

#include "libipc/platform/detail.h"
#include "libipc/utility/log.h"
#include "libipc/utility/scope_guard.h"
#include "libipc/memory/resource.h"
#include "libipc/shm.h"

#include "get_wait_time.h"

namespace ipc {
namespace detail {
namespace sync {

class mutex {
    ipc::shm::handle *shm_ = nullptr;
    std::atomic<std::int32_t> *ref_ = nullptr;
    pthread_mutex_t *mutex_ = nullptr;

    struct curr_prog {
        struct shm_data {
            ipc::shm::handle shm;
            std::atomic<std::int32_t> ref;

            struct init {
                char const *name;
                std::size_t size;
            };
            shm_data(init arg)
                : shm{arg.name, arg.size}, ref{0} {}
        };
        ipc::map<ipc::string, shm_data> mutex_handles;
        std::mutex lock;

        static curr_prog &get() {
            static curr_prog info;
            return info;
        }
    };

    pthread_mutex_t *acquire_mutex(char const *name) {
        if (name == nullptr) {
            return nullptr;
        }
        auto &info = curr_prog::get();
        IPC_UNUSED_ std::lock_guard<std::mutex> guard {info.lock};
        auto it = info.mutex_handles.find(name);
        if (it == info.mutex_handles.end()) {
            it = info.mutex_handles.emplace(name, 
                  curr_prog::shm_data::init{name, sizeof(pthread_mutex_t)}).first;
        }
        shm_ = &it->second.shm;
        ref_ = &it->second.ref;
        if (shm_ == nullptr) {
            return nullptr;
        }
        return static_cast<pthread_mutex_t *>(shm_->get());
    }

    template <typename F>
    void release_mutex(ipc::string const &name, F &&clear) {
        if (name.empty()) return;
        auto &info = curr_prog::get();
        IPC_UNUSED_ std::lock_guard<std::mutex> guard {info.lock};
        auto it = info.mutex_handles.find(name);
        if (it == info.mutex_handles.end()) {
            return;
        }
        if (clear()) {
            info.mutex_handles.erase(it);
        }
    }

    static pthread_mutex_t const &zero_mem() {
        static const pthread_mutex_t tmp{};
        return tmp;
    }

public:
    mutex() = default;
    ~mutex() = default;

    static void init() {
        // Avoid exception problems caused by static member initialization order.
        zero_mem();
        curr_prog::get();
    }

    pthread_mutex_t const *native() const noexcept {
        return mutex_;
    }

    pthread_mutex_t *native() noexcept {
        return mutex_;
    }

    bool valid() const noexcept {
        return (shm_ != nullptr) && (ref_ != nullptr) && (mutex_ != nullptr)
            && (std::memcmp(&zero_mem(), mutex_, sizeof(pthread_mutex_t)) != 0);
    }

    bool open(char const *name) noexcept {
        close();
        if ((mutex_ = acquire_mutex(name)) == nullptr) {
            return false;
        }
        auto self_ref = ref_->fetch_add(1, std::memory_order_relaxed);
        if (shm_->ref() > 1 || self_ref > 0) {
            return valid();
        }
        ::pthread_mutex_destroy(mutex_);
        auto finally = ipc::guard([this] { close(); }); // close when failed
        // init mutex
        int eno;
        pthread_mutexattr_t mutex_attr;
        if ((eno = ::pthread_mutexattr_init(&mutex_attr)) != 0) {
            ipc::error("fail pthread_mutexattr_init[%d]\n", eno);
            return false;
        }
        IPC_UNUSED_ auto guard_mutex_attr = guard([&mutex_attr] { ::pthread_mutexattr_destroy(&mutex_attr); });
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
        return valid();
    }

    void close() noexcept {
        if ((ref_ != nullptr) && (shm_ != nullptr) && (mutex_ != nullptr)) {
            if (shm_->name() != nullptr) {
                release_mutex(shm_->name(), [this] {
                    auto self_ref = ref_->fetch_sub(1, std::memory_order_relaxed);
                    if ((shm_->ref() <= 1) && (self_ref <= 1)) {
                        int eno;
                        if ((eno = ::pthread_mutex_destroy(mutex_)) != 0) {
                            ipc::error("fail pthread_mutex_destroy[%d]\n", eno);
                        }
                        return true;
                    }
                    return false;
                });
            } else shm_->release();
        }
        shm_   = nullptr;
        ref_   = nullptr;
        mutex_ = nullptr;
    }

    bool lock(std::uint64_t tm) noexcept {
        if (!valid()) return false;
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
                    if (shm_->ref() > 1) {
                        shm_->sub_ref();
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
        if (!valid()) return false;
        auto ts = detail::make_timespec(0);
        int eno = ::pthread_mutex_timedlock(mutex_, &ts);
        switch (eno) {
        case 0:
            return true;
        case ETIMEDOUT:
            return false;
        case EOWNERDEAD: {
                if (shm_->ref() > 1) {
                    shm_->sub_ref();
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
        if (!valid()) return false;
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
