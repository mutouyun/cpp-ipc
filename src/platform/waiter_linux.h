#pragma once

#include <pthread.h>

#include "def.h"
#include "log.h"

#include "platform/detail.h"

namespace ipc {
namespace detail {

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
        IPC_PTHREAD_FUNC_(pthread_mutex_lock, &mutex_);
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

    bool wait(mutex& mtx) {
        IPC_PTHREAD_FUNC_(pthread_cond_wait, &cond_, &mtx.native());
    }

    bool notify() {
        IPC_PTHREAD_FUNC_(pthread_cond_signal, &cond_);
    }

    bool broadcast() {
        IPC_PTHREAD_FUNC_(pthread_cond_broadcast, &cond_);
    }
};

#pragma pop_macro("IPC_PTHREAD_FUNC_")

class semaphore {
    mutex     lock_;
    condition cond_;
    long      counter_;

public:
    bool open(long count = 0) {
        if (lock_.open() && cond_.open()) {
            IPC_UNUSED_ auto guard = ipc::detail::unique_lock(lock_);
            counter_ = count;
            return true;
        }
        return false;
    }

    void close() {
        cond_.close();
        lock_.close();
    }

    template <typename F>
    bool wait_if(F&& check) {
        bool ret = true;
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(lock_);
        while ((counter_ <= 0) &&
               std::forward<F>(check)() &&
               (ret = cond_.wait(lock_))) ;
        -- counter_;
        return ret;
    }

    template <typename F>
    bool post(F&& count) {
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(lock_);
        auto c = std::forward<F>(count)();
        if (c <= 0) return false;
        counter_ += c;
        return cond_.broadcast();
    }
};

class waiter {
    semaphore sem_;
    std::atomic<long>     counter_ { 0 };
    std::atomic<unsigned> opened_  { 0 };

public:
    using handle_t = waiter * ;

    constexpr static handle_t invalid() {
        return nullptr;
    }

    handle_t open(char const * name) {
        if (name == nullptr || name[0] == '\0') {
            return invalid();
        }
        if ((opened_.fetch_add(1, std::memory_order_acq_rel) == 0) && !sem_.open()) {
            return invalid();
        }
        return this;
    }

    void close(handle_t h) {
        if (opened_.fetch_sub(1, std::memory_order_release) == 1) {
            if (h == invalid()) return;
            sem_.close();
        }
    }

    template <typename F>
    bool wait_if(handle_t h, F&& pred) {
        if (h == invalid()) return false;
        counter_.fetch_add(1, std::memory_order_release);
        IPC_UNUSED_ auto guard = unique_ptr(&counter_, [](decltype(counter_)* c) {
            c->fetch_sub(1, std::memory_order_release);
        });
        return sem_.wait_if(std::forward<F>(pred));
    }

    void notify(handle_t h) {
        if (h == invalid()) return;
        sem_.post([this] {
            return (0 < counter_.load(std::memory_order_relaxed)) ? 1 : 0;
        });
    }

    void broadcast(handle_t h) {
        if (h == invalid()) return;
        sem_.post([this] {
            return counter_.load(std::memory_order_relaxed);
        });
    }
};

} // namespace detail
} // namespace ipc
