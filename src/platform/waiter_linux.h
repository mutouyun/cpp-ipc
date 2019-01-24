#pragma once

#include <pthread.h>

#include <cstring>
#include <atomic>

#include "def.h"

#include "platform/detail.h"

namespace ipc {
namespace detail {

class waiter {
    pthread_mutex_t       mutex_ = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t        cond_  = PTHREAD_COND_INITIALIZER;
    std::atomic<unsigned> counter_ { 0 };

public:
    using handle_t = void*;

private:
    constexpr static waiter* waiter_cast(handle_t h) {
        return static_cast<waiter*>(h);
    }

public:
    constexpr static handle_t invalid() {
        return nullptr;
    }

    handle_t open(char const * name) {
        if (name == nullptr || name[0] == '\0') return invalid();
        if (counter_.fetch_add(1, std::memory_order_acq_rel) == 0) {
            // init mutex
            pthread_mutexattr_t mutex_attr;
            if (::pthread_mutexattr_init(&mutex_attr) != 0) {
                ::printf("fail pthread_mutexattr_init\n");
                return invalid();
            }
            IPC_UNUSED_ auto guard_mutex_attr = unique_ptr(&mutex_attr, ::pthread_mutexattr_destroy);
            if (::pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED) != 0) {
                ::printf("fail pthread_mutexattr_setpshared\n");
                return invalid();
            }
            if (::pthread_mutex_init(&mutex_, &mutex_attr) != 0) {
                ::printf("fail pthread_mutex_init\n");
                return invalid();
            }
            auto guard_mutex = unique_ptr(&mutex_, ::pthread_mutex_destroy);
            // init condition
            pthread_condattr_t cond_attr;
            if (::pthread_condattr_init(&cond_attr) != 0) {
                ::printf("fail pthread_condattr_init\n");
                return invalid();
            }
            IPC_UNUSED_ auto guard_cond_attr = unique_ptr(&cond_attr, ::pthread_condattr_destroy);
            if (::pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED) != 0) {
                ::printf("fail pthread_condattr_setpshared\n");
                return invalid();
            }
            if (::pthread_cond_init(&cond_, &cond_attr) != 0) {
                ::printf("fail pthread_cond_init\n");
                return invalid();
            }
            // no need to guard condition
            // release guards
            guard_mutex.release();
        }
        return this;
    }

    void close(handle_t h) {
        if (h == invalid()) return;
        auto w = waiter_cast(h);
        if (w->counter_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            ::pthread_cond_destroy (&(w->cond_ ));
            ::pthread_mutex_destroy(&(w->mutex_));
        }
    }

    bool wait(handle_t h) {
        if (h == invalid()) return false;
        auto w = waiter_cast(h);
        if (::pthread_mutex_lock(&(w->mutex_)) != 0) {
            return false;
        }
        IPC_UNUSED_ auto guard = unique_ptr(&(w->mutex_), ::pthread_mutex_unlock);
        if (::pthread_cond_wait(&(w->cond_), &(w->mutex_)) != 0) {
            return false;
        }
        return true;
    }

    void notify(handle_t h) {
        if (h == invalid()) return;
        ::pthread_cond_signal(&(waiter_cast(h)->cond_));
    }

    void broadcast(handle_t h) {
        if (h == invalid()) return;
        ::pthread_cond_broadcast(&(waiter_cast(h)->cond_));
    }
};

} // namespace detail
} // namespace ipc
