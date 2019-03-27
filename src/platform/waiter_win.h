#pragma once

#include <Windows.h>

#include <atomic>
#include <tuple>

#include "rw_lock.h"
#include "pool_alloc.h"
#include "log.h"
#include "shm.h"

#include "platform/to_tchar.h"
#include "platform/detail.h"

namespace ipc {
namespace detail {

class semaphore {
    HANDLE h_ = NULL;

public:
    static void remove(char const * /*name*/) {}

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

    bool wait(std::size_t tm = invalid_value) {
        DWORD ret, ms = (tm == invalid_value) ? INFINITE : static_cast<DWORD>(tm);
        if ((ret = ::WaitForSingleObject(h_, ms)) == WAIT_OBJECT_0) {
            return true;
        }
        if (ret == WAIT_TIMEOUT) {
            return false;
        }
        ipc::error("fail WaitForSingleObject[%lu]: 0x%08X\n", ::GetLastError(), ret);
        return false;
    }

    bool post(long count = 1) {
        if (::ReleaseSemaphore(h_, count, NULL)) {
            return true;
        }
        ipc::error("fail ReleaseSemaphore[%lu]\n", ::GetLastError());
        return false;
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

    std::atomic<unsigned> * waiting_ = nullptr;
    long                  * counter_ = nullptr;

public:
    friend bool operator==(condition const & c1, condition const & c2) {
        return (c1.waiting_ == c2.waiting_) && (c1.counter_ == c2.counter_);
    }

    friend bool operator!=(condition const & c1, condition const & c2) {
        return !(c1 == c2);
    }

    static void remove(char const * name) {
        semaphore::remove((std::string{ "__COND_HAN__" } + name).c_str());
        semaphore::remove((std::string{ "__COND_SEM__" } + name).c_str());
        mutex    ::remove((std::string{ "__COND_MTX__" } + name).c_str());
    }

    bool open(std::string const & name, std::atomic<unsigned> * waiting, long * counter) {
        if (lock_     .open("__COND_MTX__" + name) &&
            sema_     .open("__COND_SEM__" + name) &&
            handshake_.open("__COND_HAN__" + name)) {
            waiting_ = waiting;
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
    bool wait_if(Mutex& mtx, F&& pred, std::size_t tm = invalid_value) {
        waiting_->fetch_add(1, std::memory_order_release);
        {
            IPC_UNUSED_ auto guard = ipc::detail::unique_lock(lock_);
            if (!std::forward<F>(pred)()) return true;
            ++ *counter_;
        }
        mtx.unlock();
        bool ret = sema_.wait(tm);
        waiting_->fetch_sub(1, std::memory_order_release);
        ret = handshake_.post() && ret;
        mtx.lock();
        return ret;
    }

    bool notify() {
        std::atomic_thread_fence(std::memory_order_acq_rel);
        if (waiting_->load(std::memory_order_relaxed) == 0) {
            return true;
        }
        bool ret = true;
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(lock_);
        if (*counter_ > 0) {
            ret = sema_.post();
            -- *counter_;
            ret = ret && handshake_.wait(default_timeut);
        }
        return ret;
    }

    bool broadcast() {
        std::atomic_thread_fence(std::memory_order_acq_rel);
        if (waiting_->load(std::memory_order_relaxed) == 0) {
            return true;
        }
        bool ret = true;
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(lock_);
        if (*counter_ > 0) {
            ret = sema_.post(*counter_);
            do {
                -- *counter_;
                ret = ret && handshake_.wait(default_timeut);
            } while (*counter_ > 0);
        }
        return ret;
    }
};

class waiter {

    std::atomic<unsigned> waiting_ { 0 };
    long                  counter_ = 0;

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
        if (cond.open(name, &waiting_, &counter_)) {
            return cond;
        }
        return invalid();
    }

    void close(handle_t& h) {
        if (h == invalid()) return;
        h.close();
    }

    template <typename F>
    bool wait_if(handle_t& h, F&& pred, std::size_t tm = invalid_value) {
        if (h == invalid()) return false;

        class non_mutex {
        public:
            void lock  () noexcept {}
            void unlock() noexcept {}
        } nm;

        return h.wait_if(nm, std::forward<F>(pred), tm);
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
