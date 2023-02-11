#pragma once

#include <cstdint>
#include <system_error>
#include <mutex>
#include <atomic>

#include "libipc/platform/detail.h"
#include "libipc/utility/log.h"
#include "libipc/memory/resource.h"
#include "libipc/shm.h"

#include "get_wait_time.h"
#include "sync_obj_impl.h"

#include "a0/err_macro.h"
#include "a0/mtx.h"

namespace ipc {
namespace detail {
namespace sync {

class robust_mutex : public sync::obj_impl<a0_mtx_t> {
public:
    bool lock(std::uint64_t tm) noexcept {
        if (!valid()) return false;
        for (;;) {
            auto ts = detail::make_timespec(tm);
            int eno = A0_SYSERR(
                (tm == invalid_value) ? a0_mtx_lock(native()) 
                                      : a0_mtx_timedlock(native(), {ts}));
            switch (eno) {
            case 0:
                return true;
            case ETIMEDOUT:
                return false;
            case EOWNERDEAD: {
                    int eno2 = A0_SYSERR(a0_mtx_consistent(native()));
                    if (eno2 != 0) {
                        ipc::error("fail mutex lock[%d] -> consistent[%d]\n", eno, eno2);
                        return false;
                    }
                    int eno3 = A0_SYSERR(a0_mtx_unlock(native()));
                    if (eno3 != 0) {
                        ipc::error("fail mutex lock[%d] -> unlock[%d]\n", eno, eno3);
                        return false;
                    }
                }
                break; // loop again
            default:
                ipc::error("fail mutex lock[%d]\n", eno);
                return false;
            }
        }
    }

    bool try_lock() noexcept(false) {
        if (!valid()) return false;
        int eno = A0_SYSERR(a0_mtx_timedlock(native(), {detail::make_timespec(0)}));
        switch (eno) {
        case 0:
            return true;
        case ETIMEDOUT:
            return false;
        case EOWNERDEAD: {
                int eno2 = A0_SYSERR(a0_mtx_consistent(native()));
                if (eno2 != 0) {
                    ipc::error("fail mutex try_lock[%d] -> consistent[%d]\n", eno, eno2);
                    break;
                }
                int eno3 = A0_SYSERR(a0_mtx_unlock(native()));
                if (eno3 != 0) {
                    ipc::error("fail mutex try_lock[%d] -> unlock[%d]\n", eno, eno3);
                    break;
                }
            }
            break;
        default:
            ipc::error("fail mutex try_lock[%d]\n", eno);
            break;
        }
        throw std::system_error{eno, std::system_category()};
    }

    bool unlock() noexcept {
        if (!valid()) return false;
        int eno = A0_SYSERR(a0_mtx_unlock(native()));
        if (eno != 0) {
            ipc::error("fail mutex unlock[%d]\n", eno);
            return false;
        }
        return true;
    }
};

class mutex {
    robust_mutex *mutex_ = nullptr;
    std::atomic<std::int32_t> *ref_ = nullptr;

    struct curr_prog {
        struct shm_data {
            robust_mutex mtx;
            std::atomic<std::int32_t> ref;

            struct init {
                char const *name;
            };
            shm_data(init arg)
                : mtx{}, ref{0} { mtx.open(arg.name); }
        };
        ipc::map<ipc::string, shm_data> mutex_handles;
        std::mutex lock;

        static curr_prog &get() {
            static curr_prog info;
            return info;
        }
    };

    void acquire_mutex(char const *name) {
        if (name == nullptr) {
            return;
        }
        auto &info = curr_prog::get();
        IPC_UNUSED_ std::lock_guard<std::mutex> guard {info.lock};
        auto it = info.mutex_handles.find(name);
        if (it == info.mutex_handles.end()) {
            it = info.mutex_handles.emplace(name, 
                  curr_prog::shm_data::init{name}).first;
        }
        mutex_ = &it->second.mtx;
        ref_   = &it->second.ref;
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

public:
    mutex() = default;
    ~mutex() = default;

    static void init() {
        // Avoid exception problems caused by static member initialization order.
        curr_prog::get();
    }

    a0_mtx_t const *native() const noexcept {
        return valid() ? mutex_->native() : nullptr;
    }

    a0_mtx_t *native() noexcept {
        return valid() ? mutex_->native() : nullptr;
    }

    bool valid() const noexcept {
        return (mutex_ != nullptr) && (ref_ != nullptr) && mutex_->valid();
    }

    bool open(char const *name) noexcept {
        close();
        acquire_mutex(name);
        if (!valid()) {
            return false;
        }
        ref_->fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    void close() noexcept {
        if ((mutex_ != nullptr) && (ref_ != nullptr)) {
            if (mutex_->name() != nullptr) {
                release_mutex(mutex_->name(), [this] {
                    return ref_->fetch_sub(1, std::memory_order_relaxed) <= 1;
                });
            } else mutex_->close();
        }
        mutex_ = nullptr;
        ref_   = nullptr;
    }

    bool lock(std::uint64_t tm) noexcept {
        if (!valid()) return false;
        return mutex_->lock(tm);
    }

    bool try_lock() noexcept(false) {
        if (!valid()) return false;
        return mutex_->try_lock();
    }

    bool unlock() noexcept {
        if (!valid()) return false;
        return mutex_->unlock();
    }
};

} // namespace sync
} // namespace detail
} // namespace ipc
