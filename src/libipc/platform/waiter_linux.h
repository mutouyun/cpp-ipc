#pragma once

#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <semaphore.h>
#include <errno.h>

#include <atomic>
#include <tuple>
#include <utility>
#include <cassert>

#include "libipc/def.h"
#include "libipc/waiter_helper.h"

#include "libipc/utility/log.h"
#include "libipc/platform/detail.h"
#include "libipc/memory/resource.h"

namespace ipc {
namespace detail {

inline static bool calc_wait_time(timespec& ts, std::size_t tm /*ms*/) {
    timeval now;
    int eno = ::gettimeofday(&now, NULL);
    if (eno != 0) {
        ipc::error("fail gettimeofday [%d]\n", eno);
        return false;
    }
    ts.tv_nsec = (now.tv_usec + (tm % 1000) * 1000) * 1000;
    ts.tv_sec  =  now.tv_sec  + (tm / 1000) + (ts.tv_nsec / 1000000000);
    ts.tv_nsec %= 1000000000;
    return true;
}

#pragma push_macro("IPC_PTHREAD_FUNC_")
#undef  IPC_PTHREAD_FUNC_
#define IPC_PTHREAD_FUNC_(CALL, ...)                \
    int eno;                                        \
    if ((eno = ::CALL(__VA_ARGS__)) != 0) {         \
        ipc::error("fail " #CALL " [%d]\n", eno);   \
        return false;                               \
    }                                               \
    return true

class mutex {
    pthread_mutex_t mutex_ = PTHREAD_MUTEX_INITIALIZER;

public:
    pthread_mutex_t& native() {
        return mutex_;
    }

    bool open() {
        int eno;
        // init mutex
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
        if ((eno = ::pthread_mutex_init(&mutex_, &mutex_attr)) != 0) {
            ipc::error("fail pthread_mutex_init[%d]\n", eno);
            return false;
        }
        return true;
    }

    bool close() {
        IPC_PTHREAD_FUNC_(pthread_mutex_destroy, &mutex_);
    }

    bool lock() {
        for (;;) {
            int eno = ::pthread_mutex_lock(&mutex_);
            switch (eno) {
            case 0:
                return true;
            case EOWNERDEAD:
                if (::pthread_mutex_consistent(&mutex_) == 0) {
                    ::pthread_mutex_unlock(&mutex_);
                    break;
                }
                IPC_FALLTHROUGH_;
            case ENOTRECOVERABLE:
                if (close() && open()) {
                    break;
                }
                IPC_FALLTHROUGH_;
            default:
                ipc::error("fail pthread_mutex_lock[%d]\n", eno);
                return false;
            }
        }
    }

    bool unlock() {
        IPC_PTHREAD_FUNC_(pthread_mutex_unlock, &mutex_);
    }
};

class condition {
    pthread_cond_t cond_ = PTHREAD_COND_INITIALIZER;

public:
    bool open() {
        int eno;
        // init condition
        pthread_condattr_t cond_attr;
        if ((eno = ::pthread_condattr_init(&cond_attr)) != 0) {
            ipc::error("fail pthread_condattr_init[%d]\n", eno);
            return false;
        }
        IPC_UNUSED_ auto guard_cond_attr = unique_ptr(&cond_attr, ::pthread_condattr_destroy);
        if ((eno = ::pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED)) != 0) {
            ipc::error("fail pthread_condattr_setpshared[%d]\n", eno);
            return false;
        }
        if ((eno = ::pthread_cond_init(&cond_, &cond_attr)) != 0) {
            ipc::error("fail pthread_cond_init[%d]\n", eno);
            return false;
        }
        return true;
    }

    bool close() {
        IPC_PTHREAD_FUNC_(pthread_cond_destroy, &cond_);
    }

    bool wait(mutex& mtx, std::size_t tm = invalid_value) {
        switch (tm) {
        case 0:
            return true;
        case invalid_value:
            IPC_PTHREAD_FUNC_(pthread_cond_wait, &cond_, &mtx.native());
        default: {
                timespec ts;
                if (!calc_wait_time(ts, tm)) {
                    ipc::error("fail calc_wait_time: tm = %zd, tv_sec = %ld, tv_nsec = %ld\n",
                               tm, ts.tv_sec, ts.tv_nsec);
                    return false;
                }
                int eno;
                if ((eno = ::pthread_cond_timedwait(&cond_, &mtx.native(), &ts)) != 0) {
                    if (eno != ETIMEDOUT) {
                        ipc::error("fail pthread_cond_timedwait[%d]: tm = %zd, tv_sec = %ld, tv_nsec = %ld\n",
                                   eno, tm, ts.tv_sec, ts.tv_nsec);
                    }
                    return false;
                }
            }
            return true;
        }
    }

    bool notify() {
        IPC_PTHREAD_FUNC_(pthread_cond_signal, &cond_);
    }

    bool broadcast() {
        IPC_PTHREAD_FUNC_(pthread_cond_broadcast, &cond_);
    }
};

#pragma pop_macro("IPC_PTHREAD_FUNC_")

class sem_helper {
public:
    using handle_t = sem_t*;

    constexpr static handle_t invalid() noexcept {
        return SEM_FAILED;
    }

    static handle_t open(char const * name, long count) {
        handle_t sem = ::sem_open(name, O_CREAT, 0666, count);
        if (sem == SEM_FAILED) {
            ipc::error("fail sem_open[%d]: %s\n", errno, name);
            return invalid();
        }
        return sem;
    }

#pragma push_macro("IPC_SEMAPHORE_FUNC_")
#undef  IPC_SEMAPHORE_FUNC_
#define IPC_SEMAPHORE_FUNC_(CALL, ...)              \
    if (::CALL(__VA_ARGS__) != 0) {                 \
        ipc::error("fail " #CALL "[%d]\n", errno);  \
        return false;                               \
    }                                               \
    return true

    static bool close(handle_t h) {
        if (h == invalid()) return false;
        IPC_SEMAPHORE_FUNC_(sem_close, h);
    }

    static bool destroy(char const * name) {
        IPC_SEMAPHORE_FUNC_(sem_unlink, name);
    }

    static bool post(handle_t h, long count) {
        if (h == invalid()) return false;
        auto spost = [](handle_t h) {
            IPC_SEMAPHORE_FUNC_(sem_post, h);
        };
        for (long i = 0; i < count; ++i) {
            if (!spost(h)) return false;
        }
        return true;
    }

    static bool wait(handle_t h, std::size_t tm = invalid_value) {
        if (h == invalid()) return false;
        switch (tm) {
        case 0:
            return true;
        case invalid_value:
            IPC_SEMAPHORE_FUNC_(sem_wait, h);
        default: {
                timespec ts;
                if (!calc_wait_time(ts, tm)) {
                    ipc::error("fail calc_wait_time: tm = %zd, tv_sec = %ld, tv_nsec = %ld\n",
                               tm, ts.tv_sec, ts.tv_nsec);
                    return false;
                }
                if (::sem_timedwait(h, &ts) != 0) {
                    if (errno != ETIMEDOUT) {
                        ipc::error("fail sem_timedwait [%d]: tm = %zd, tv_sec = %ld, tv_nsec = %ld\n",
                                   errno, tm, ts.tv_sec, ts.tv_nsec);
                    }
                    return false;
                }
            }
            return true;
        }
    }

#pragma pop_macro("IPC_SEMAPHORE_FUNC_")
};

class waiter_holder {
public:
    using handle_t = std::tuple<
        ipc::string, 
        sem_helper::handle_t /* sema */, 
        sem_helper::handle_t /* handshake */>;

    static handle_t invalid() noexcept {
        return std::make_tuple(
            ipc::string{}, 
            sem_helper::invalid(), 
            sem_helper::invalid());
    }

private:
    using wait_flags   = waiter_helper::wait_flags;
    using wait_counter = waiter_helper::wait_counter;

    mutex lock_;
    wait_counter cnt_;

    struct contrl {
        waiter_holder * me_;
        wait_flags * flags_;
        handle_t const & h_;

        wait_flags & flags() noexcept {
            assert(flags_ != nullptr);
            return *flags_;
        }

        wait_counter & counter() noexcept {
            return me_->cnt_;
        }

        auto get_lock() {
            return ipc::detail::unique_lock(me_->lock_);
        }

        bool sema_wait(std::size_t tm) {
            return sem_helper::wait(std::get<1>(h_), tm);
        }

        bool sema_post(long count) {
            return sem_helper::post(std::get<1>(h_), count);
        }

        bool handshake_wait(std::size_t tm) {
            return sem_helper::wait(std::get<2>(h_), tm);
        }

        bool handshake_post(long count) {
            return sem_helper::post(std::get<2>(h_), count);
        }
    };

public:
    handle_t open_h(ipc::string && name) {
        auto sem = sem_helper::open(("__WAITER_HELPER_SEM__" + name).c_str(), 0);
        if (sem == sem_helper::invalid()) {
            return invalid();
        }
        auto han = sem_helper::open(("__WAITER_HELPER_HAN__" + name).c_str(), 0);
        if (han == sem_helper::invalid()) {
            return invalid();
        }
        return std::make_tuple(std::move(name), sem, han);
    }

    void release_h(handle_t const & h) {
        sem_helper::close(std::get<2>(h));
        sem_helper::close(std::get<1>(h));
    }

    void close_h(handle_t const & h) {
        auto const & name = std::get<0>(h);
        sem_helper::destroy(("__WAITER_HELPER_HAN__" + name).c_str());
        sem_helper::destroy(("__WAITER_HELPER_SEM__" + name).c_str());
    }

    bool open() {
        return lock_.open();
    }

    void close() {
        lock_.close();
    }

    template <typename F>
    bool wait_if(handle_t const & h, wait_flags * flags, F&& pred, std::size_t tm = invalid_value) {
        assert(flags != nullptr);
        contrl ctrl { this, flags, h };

        class non_mutex {
        public:
            void lock  () noexcept {}
            void unlock() noexcept {}
        } nm;

        return waiter_helper::wait_if(ctrl, nm, std::forward<F>(pred), tm);
    }

    bool notify(handle_t const & h) {
        contrl ctrl { this, nullptr, h };
        return waiter_helper::notify(ctrl);
    }

    bool broadcast(handle_t const & h) {
        contrl ctrl { this, nullptr, h };
        return waiter_helper::broadcast(ctrl);
    }

    bool quit_waiting(handle_t const & h, wait_flags * flags) {
        assert(flags != nullptr);
        contrl ctrl { this, flags, h };
        return waiter_helper::quit_waiting(ctrl);
    }
};

class waiter {
    waiter_holder helper_;
    std::atomic<unsigned> opened_ { 0 };

public:
    using handle_t = waiter_holder::handle_t;

    static handle_t invalid() noexcept {
        return waiter_holder::invalid();
    }

    handle_t open(char const * name) {
        if (name == nullptr || name[0] == '\0') {
            return invalid();
        }
        if ((opened_.fetch_add(1, std::memory_order_acq_rel) == 0) && !helper_.open()) {
            return invalid();
        }
        return helper_.open_h(name);
    }

    void close(handle_t h) {
        if (h == invalid()) return;
        helper_.release_h(h);
        if (opened_.fetch_sub(1, std::memory_order_release) == 1) {
            helper_.close_h(h);
            helper_.close();
        }
    }

    template <typename F>
    bool wait_if(handle_t h, waiter_helper::wait_flags * flags, F && pred, std::size_t tm = invalid_value) {
        if (h == invalid()) return false;
        return helper_.wait_if(h, flags, std::forward<F>(pred), tm);
    }

    bool notify(handle_t h) {
        if (h == invalid()) return false;
        return helper_.notify(h);
    }

    bool broadcast(handle_t h) {
        if (h == invalid()) return false;
        return helper_.broadcast(h);
    }

    bool quit_waiting(handle_t h, waiter_helper::wait_flags * flags) {
        if (h == invalid()) return false;
        return helper_.quit_waiting(h, flags);
    }
};

} // namespace detail
} // namespace ipc
