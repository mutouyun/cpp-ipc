#pragma once

#include <Windows.h>

#include <atomic>
#include <tuple>

#include "rw_lock.h"
#include "pool_alloc.h"

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

    template <typename Str>
    semaphore& open(Str const & name, long count = 0, long limit = LONG_MAX) {
        h_ = ::CreateSemaphore(NULL, count, limit, name.c_str());
        return *this;
    }

    void close() {
        ::CloseHandle(h_);
    }

    bool wait() {
        return ::WaitForSingleObject(h_, INFINITE) == WAIT_OBJECT_0;
    }

    bool post(long count = 1) {
        if (count <= 0) return true;
        return !!::ReleaseSemaphore(h_, count, NULL);
    }
};

class mutex : public semaphore {
    using semaphore::wait;
    using semaphore::post;

public:
    template <typename Str>
    mutex& open(Str const & name) {
        semaphore::open(name, 1, 1);
        return *this;
    }

    bool lock  () { return semaphore::wait(); }
    bool unlock() { return semaphore::post(); }
};

class waiter {
    long volatile counter_ = 0;

public:
    using handle_t = std::tuple<semaphore, semaphore, mutex>;

    static handle_t invalid() {
        return handle_t {};
    }

private:
    semaphore& sem(handle_t& h) { return std::get<0>(h); }
    semaphore& han(handle_t& h) { return std::get<1>(h); }
    mutex    & mtx(handle_t& h) { return std::get<2>(h); }

public:
    handle_t open(char const * name) {
        if (name == nullptr || name[0] == '\0') return invalid();
        std::string n = name;
        return handle_t {
            semaphore {}.open(ipc::detail::to_tchar(n + "__SEM__")),
            semaphore {}.open(ipc::detail::to_tchar(n + "__HAN__")),
            mutex     {}.open(ipc::detail::to_tchar(n + "__MTX__"))
        };
    }

    void close(handle_t& h) {
        if (h == invalid()) return;
        mtx(h).close();
        han(h).close();
        sem(h).close();
    }

    template <typename F>
    bool wait_if(handle_t& h, F&& pred) {
        if (h == invalid()) return false;
        {
            IPC_UNUSED_ auto guard = ipc::detail::unique_lock(mtx(h));
            if (!std::forward<F>(pred)()) return true;
            ++ counter_;
        }
        bool ret_s = sem(h).wait();
        bool ret_h = han(h).post();
        return ret_s && ret_h;
    }

    void notify(handle_t& h) {
        if (h == invalid()) return;
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(mtx(h));
        if (counter_ > 0) {
            sem(h).post();
            -- counter_;
            han(h).wait();
        }
    }

    void broadcast(handle_t& h) {
        if (h == invalid()) return;
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(mtx(h));
        sem(h).post(counter_);
        while (counter_ > 0) {
            -- counter_;
            han(h).wait();
        }
    }
};

} // namespace detail
} // namespace ipc
