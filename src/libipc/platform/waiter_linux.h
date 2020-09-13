#pragma once

#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <semaphore.h>
#include <errno.h>

#include <atomic>
#include <tuple>

#include "libipc/def.h"

#include "libipc/utility/log.h"
#include "libipc/platform/detail.h"
#include "libipc/memory/resource.h"

namespace ipc {
namespace detail {

inline static bool calc_wait_time(timespec& ts, std::size_t tm /*ms*/) {
    timeval now;
    int eno = ::gettimeofday(&now, NULL);
    if (eno != 0) {
        ipc::error("fail gettimeofday[%d]\n", eno);
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
        ipc::error("fail " #CALL "[%d]\n", eno);    \
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
                calc_wait_time(ts, tm);
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

    static handle_t open(char const* name, long count) {
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

    static bool destroy(char const* name) {
        IPC_SEMAPHORE_FUNC_(sem_unlink, name);
    }

    static bool post(handle_t h) {
        if (h == invalid()) return false;
        IPC_SEMAPHORE_FUNC_(sem_post, h);
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
                calc_wait_time(ts, tm);
                if (::sem_timedwait(h, &ts) != 0) {
                    if (errno != ETIMEDOUT) {
                        ipc::error("fail sem_timedwait[%d]: tm = %zd, tv_sec = %ld, tv_nsec = %ld\n",
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

class waiter_helper {
    mutex lock_;

    std::atomic<unsigned> waiting_ { 0 };
    long counter_ = 0;

public:
    using handle_t = std::tuple<ipc::string, sem_helper::handle_t, sem_helper::handle_t>;

    static handle_t invalid() noexcept {
        return std::make_tuple(ipc::string{}, sem_helper::invalid(), sem_helper::invalid());
    }

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
    bool wait_if(handle_t const & h, F&& pred, std::size_t tm = invalid_value) {
        waiting_.fetch_add(1, std::memory_order_release);
        {
            IPC_UNUSED_ auto guard = ipc::detail::unique_lock(lock_);
            if (!std::forward<F>(pred)()) return true;
            ++ counter_;
        }
        bool ret = sem_helper::wait(std::get<1>(h), tm);
        waiting_.fetch_sub(1, std::memory_order_release);
        ret = sem_helper::post(std::get<2>(h)) && ret;
        return ret;
    }

    bool notify(handle_t const & h) {
        std::atomic_thread_fence(std::memory_order_acq_rel);
        if (waiting_.load(std::memory_order_relaxed) == 0) {
            return true;
        }
        bool ret = true;
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(lock_);
        if (counter_ > 0) {
            ret = sem_helper::post(std::get<1>(h));
            -- counter_;
            ret = ret && sem_helper::wait(std::get<2>(h), default_timeout);
        }
        return ret;
    }

    bool broadcast(handle_t const & h) {
        std::atomic_thread_fence(std::memory_order_acq_rel);
        if (waiting_.load(std::memory_order_relaxed) == 0) {
            return true;
        }
        bool ret = true;
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(lock_);
        if (counter_ > 0) {
            for (long i = 0; i < counter_; ++i) {
                ret = ret && sem_helper::post(std::get<1>(h));
            }
            do {
                -- counter_;
                ret = ret && sem_helper::wait(std::get<2>(h), default_timeout);
            } while (counter_ > 0);
        }
        return ret;
    }
};

class waiter {
    waiter_helper helper_;
    std::atomic<unsigned> opened_ { 0 };

public:
    using handle_t = waiter_helper::handle_t;

    static handle_t invalid() noexcept {
        return waiter_helper::invalid();
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
    bool wait_if(handle_t h, F&& pred, std::size_t tm = invalid_value) {
        if (h == invalid()) return false;
        return helper_.wait_if(h, std::forward<F>(pred), tm);
    }

    void notify(handle_t h) {
        if (h == invalid()) return;
        helper_.notify(h);
    }

    void broadcast(handle_t h) {
        if (h == invalid()) return;
        helper_.broadcast(h);
    }
};

} // namespace detail
} // namespace ipc
