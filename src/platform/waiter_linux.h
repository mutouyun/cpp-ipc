#pragma once

#include <pthread.h>

#include <cstring>
#include <atomic>

#include "def.h"
#include "log.h"
#include "platform/detail.h"

namespace ipc {
namespace detail {

class waiter {
    pthread_mutex_t       mutex_ = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t        cond_  = PTHREAD_COND_INITIALIZER;
    std::atomic<unsigned> counter_ { 0 };

public:
    using handle_t = bool;

public:
    constexpr static handle_t invalid() {
        return false;
    }

    handle_t open(char const * name) {
        if (name == nullptr || name[0] == '\0') return invalid();
        if (counter_.fetch_add(1, std::memory_order_acq_rel) == 0) {
            int eno;
            // init mutex
            pthread_mutexattr_t mutex_attr;
            if ((eno = ::pthread_mutexattr_init(&mutex_attr)) != 0) {
                ipc::log("fail pthread_mutexattr_init[%d]: %s\n", eno, name);
                return invalid();
            }
            IPC_UNUSED_ auto guard_mutex_attr = unique_ptr(&mutex_attr, ::pthread_mutexattr_destroy);
            if ((eno = ::pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED)) != 0) {
                ipc::log("fail pthread_mutexattr_setpshared[%d]: %s\n", eno, name);
                return invalid();
            }
            if ((eno = ::pthread_mutex_init(&mutex_, &mutex_attr)) != 0) {
                ipc::log("fail pthread_mutex_init[%d]: %s\n", eno, name);
                return invalid();
            }
            auto guard_mutex = unique_ptr(&mutex_, ::pthread_mutex_destroy);
            // init condition
            pthread_condattr_t cond_attr;
            if ((eno = ::pthread_condattr_init(&cond_attr)) != 0) {
                ipc::log("fail pthread_condattr_init[%d]: %s\n", eno, name);
                return invalid();
            }
            IPC_UNUSED_ auto guard_cond_attr = unique_ptr(&cond_attr, ::pthread_condattr_destroy);
            if ((eno = ::pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED)) != 0) {
                ipc::log("fail pthread_condattr_setpshared[%d]: %s\n", eno, name);
                return invalid();
            }
            if ((eno = ::pthread_cond_init(&cond_, &cond_attr)) != 0) {
                ipc::log("fail pthread_cond_init[%d]: %s\n", eno, name);
                return invalid();
            }
            // no need to guard condition
            // release guards
            guard_mutex.release();
        }
        return true;
    }

    void close(handle_t h) {
        if (h == invalid()) return;
        if (counter_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            ::pthread_cond_destroy(&cond_);
            ::pthread_mutex_destroy(&mutex_);
        }
    }

    bool wait(handle_t h) {
        if (h == invalid()) return false;
        int eno;
        if ((eno = ::pthread_mutex_lock(&mutex_)) != 0) {
            ipc::log("fail pthread_mutex_lock[%d]\n", eno);
            return false;
        }
        IPC_UNUSED_ auto guard = unique_ptr(&mutex_, ::pthread_mutex_unlock);
        if ((eno = ::pthread_cond_wait(&cond_, &mutex_)) != 0) {
            ipc::log("fail pthread_cond_wait[%d]\n", eno);
            return false;
        }
        return true;
    }

    void notify(handle_t h) {
        if (h == invalid()) return;
        int eno;
        if ((eno = ::pthread_cond_signal(&cond_)) != 0) {
            ipc::log("fail pthread_cond_signal[%d]\n", eno);
        }
    }

    void broadcast(handle_t h) {
        if (h == invalid()) return;
        int eno;
        if ((eno = ::pthread_cond_broadcast(&cond_)) != 0) {
            ipc::log("fail pthread_cond_broadcast[%d]\n", eno);
        }
    }
};

} // namespace detail
} // namespace ipc
