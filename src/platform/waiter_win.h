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
    semaphore& s(handle_t& h) { return std::get<0>(h); }
    semaphore& w(handle_t& h) { return std::get<1>(h); }
    mutex    & x(handle_t& h) { return std::get<2>(h); }

public:
    handle_t open(char const * name) {
        if (name == nullptr || name[0] == '\0') return invalid();
        std::string n = name;
        return handle_t {
            semaphore {}.open(ipc::detail::to_tchar(n + "__S__")),
            semaphore {}.open(ipc::detail::to_tchar(n + "__W__")),
            mutex     {}.open(ipc::detail::to_tchar(n + "__X__"))
        };
    }

    void close(handle_t& h) {
        if (h == invalid()) return;
        x(h).close();
        w(h).close();
        s(h).close();
    }

    template <typename F>
    bool wait_if(handle_t& h, F&& pred) {
        if (h == invalid()) return false;
        {
            IPC_UNUSED_ auto guard = ipc::detail::unique_lock(x(h));
            if (!std::forward<F>(pred)()) return true;
            ++ counter_;
        }
        if (!s(h).wait()) return false;
        return w(h).post();
    }

    void notify(handle_t& h) {
        if (h == invalid()) return;
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(x(h));
        if (counter_ > 0) {
            s(h).post();
            -- counter_;
            w(h).wait();
        }
    }

    void broadcast(handle_t& h) {
        if (h == invalid()) return;
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(x(h));
        s(h).post(counter_);
        while (counter_ > 0) {
            -- counter_;
            w(h).wait();
        }
    }
};

} // namespace detail
} // namespace ipc
