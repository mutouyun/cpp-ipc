#pragma once

#include <Windows.h>

#include <atomic>
#include <tuple>

#include "rw_lock.h"
#include "pool_alloc.h"
#include "log.h"

#include "platform/to_tchar.h"
#include "platform/detail.h"

namespace ipc {
namespace detail {

class semaphore {
    HANDLE h_ = NULL;

public:
    friend bool operator==(semaphore const & s1, semaphore const & s2) {
        return s1.h_ == s2.h_;
    }

    bool open(std::string && name, long count = 0, long limit = LONG_MAX) {
        h_ = ::CreateSemaphore(NULL, count, limit, ipc::detail::to_tchar(std::move(name)).c_str());
        if (h_ == NULL) {
            ipc::error("fail CreateSemaphore[%lu]: %s\n", ::GetLastError(), name.c_str());
            return false;
        }
        return true;
    }

    void close() {
        ::CloseHandle(h_);
    }

    bool wait() {
        return ::WaitForSingleObject(h_, INFINITE) == WAIT_OBJECT_0;
    }

    bool post(long count = 1) {
        return !!::ReleaseSemaphore(h_, count, NULL);
    }
};

class mutex : public semaphore {
    using semaphore::wait;
    using semaphore::post;

public:
    bool open(std::string && name) {
        return semaphore::open(std::move(name), 1, 1);
    }

    bool lock  () { return semaphore::wait(); }
    bool unlock() { return semaphore::post(); }
};

class condition {
    mutex     lock_;
    semaphore sema_, handshake_;

    long * counter_ = nullptr;

public:
    friend bool operator==(condition const & c1, condition const & c2) {
        return c1.counter_ == c2.counter_;
    }

    friend bool operator!=(condition const & c1, condition const & c2) {
        return !(c1 == c2);
    }

    bool open(std::string const & name, long * counter) {
        if (lock_     .open(name + "__COND_MTX__") &&
            sema_     .open(name + "__COND_SEM__") &&
            handshake_.open(name + "__COND_HAN__")) {
            counter_ = counter;
            return true;
        }
        return false;
    }

    void close() {
        handshake_.close();
        sema_     .close();
        lock_     .close();
    }

    template <typename Mutex, typename F>
    bool wait_if(Mutex& mtx, F&& pred) {
        {
            IPC_UNUSED_ auto guard = ipc::detail::unique_lock(lock_);
            if (!std::forward<F>(pred)()) return true;
            ++ *counter_;
        }
        mtx.unlock();
        bool ret_s = sema_.wait();
        bool ret_h = handshake_.post();
        mtx.lock();
        return ret_s && ret_h;
    }

    bool notify() {
        bool ret_s = true, ret_h = true;
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(lock_);
        if (*counter_ > 0) {
            ret_s = sema_.post();
            -- *counter_;
            ret_h = handshake_.wait();
        }
        return ret_s && ret_h;
    }

    bool broadcast() {
        bool ret_s = true, ret_h = true;
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(lock_);
        if (*counter_ > 0) {
            ret_s = sema_.post(*counter_);
            do {
                -- *counter_;
                bool rh = handshake_.wait();
                ret_h = ret_h && rh;
            } while (*counter_ > 0);
        }
        return ret_s && ret_h;
    }
};

class waiter {
    long counter_ = 0;

public:
    using handle_t = condition;

    static handle_t invalid() {
        return condition {};
    }

    handle_t open(char const * name) {
        if (name == nullptr || name[0] == '\0') {
            return invalid();
        }
        condition cond;
        if (cond.open(name, &counter_)) {
            return cond;
        }
        return invalid();
    }

    void close(handle_t& h) {
        if (h == invalid()) return;
        h.close();
    }

    template <typename F>
    bool wait_if(handle_t& h, F&& pred) {
        if (h == invalid()) return false;

        class non_mutex {
        public:
            void lock  () {}
            void unlock() {}
        } nm;

        return h.wait_if(nm, std::forward<F>(pred));
    }

    void notify(handle_t& h) {
        if (h == invalid()) return;
        h.notify();
    }

    void broadcast(handle_t& h) {
        if (h == invalid()) return;
        h.broadcast();
    }
};

} // namespace detail
} // namespace ipc
