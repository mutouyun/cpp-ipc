#pragma once

#include <Windows.h>

#include <atomic>
#include <utility>
#include <limits>
#include <cassert>

#include "libipc/rw_lock.h"
#include "libipc/pool_alloc.h"
#include "libipc/shm.h"
#include "libipc/waiter_helper.h"

#include "libipc/utility/log.h"
#include "libipc/utility/scope_guard.h"
#include "libipc/platform/to_tchar.h"
#include "libipc/platform/get_sa.h"
#include "libipc/platform/detail.h"
#include "libipc/memory/resource.h"

namespace ipc {
namespace detail {

class semaphore {
    HANDLE h_ = NULL;

public:
    static void remove(char const * /*name*/) {}

    bool open(ipc::string && name, long count = 0, long limit = LONG_MAX) {
        h_ = ::CreateSemaphore(detail::get_sa(), count, limit, ipc::detail::to_tchar(std::move(name)).c_str());
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
        switch ((ret = ::WaitForSingleObject(h_, ms))) {
        case WAIT_OBJECT_0:
            return true;
        case WAIT_TIMEOUT:
            return false;
        case WAIT_ABANDONED:
        default:
            ipc::error("fail WaitForSingleObject[%lu]: 0x%08X\n", ::GetLastError(), ret);
            return false;
        }
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
    bool open(ipc::string && name) {
        return semaphore::open(std::move(name), 1, 1);
    }

    bool lock  () { return semaphore::wait(); }
    bool unlock() { return semaphore::post(); }
};

class condition {
    using wait_flags   = waiter_helper::wait_flags;
    using wait_counter = waiter_helper::wait_counter;

    mutex     lock_;
    semaphore sema_, handshake_;
    wait_counter * cnt_ = nullptr;

    struct contrl {
        condition * me_;
        wait_flags * flags_;

        wait_flags & flags() noexcept {
            assert(flags_ != nullptr);
            return *flags_;
        }

        wait_counter & counter() noexcept {
            assert(me_->cnt_ != nullptr);
            return *(me_->cnt_);
        }

        auto get_lock() {
            return ipc::detail::unique_lock(me_->lock_);
        }

        bool sema_wait(std::size_t tm) {
            return me_->sema_.wait(tm);
        }

        bool sema_post(long count) {
            return me_->sema_.post(count);
        }

        bool handshake_wait(std::size_t tm) {
            return me_->handshake_.wait(tm);
        }

        bool handshake_post(long count) {
            return me_->handshake_.post(count);
        }
    };

public:
    friend bool operator==(condition const & c1, condition const & c2) {
        return c1.cnt_ == c2.cnt_;
    }

    friend bool operator!=(condition const & c1, condition const & c2) {
        return !(c1 == c2);
    }

    static void remove(char const * name) {
        semaphore::remove((ipc::string{ "__COND_HAN__" } + name).c_str());
        semaphore::remove((ipc::string{ "__COND_SEM__" } + name).c_str());
        mutex    ::remove((ipc::string{ "__COND_MTX__" } + name).c_str());
    }

    bool open(ipc::string const & name, wait_counter * cnt) {
        if (lock_     .open("__COND_MTX__" + name) &&
            sema_     .open("__COND_SEM__" + name) &&
            handshake_.open("__COND_HAN__" + name)) {
            cnt_ = cnt;
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
    bool wait_if(Mutex & mtx, wait_flags * flags, F && pred, std::size_t tm = invalid_value) {
        assert(flags != nullptr);
        contrl ctrl { this, flags };
        return waiter_helper::wait_if(ctrl, mtx, std::forward<F>(pred), tm);
    }

    bool notify() {
        contrl ctrl { this, nullptr };
        return waiter_helper::notify(ctrl);
    }

    bool broadcast() {
        contrl ctrl { this, nullptr };
        return waiter_helper::broadcast(ctrl);
    }

    bool quit_waiting(wait_flags * flags) {
        assert(flags != nullptr);
        contrl ctrl { this, flags };
        return waiter_helper::quit_waiting(ctrl);
    }
};

class waiter {
    waiter_helper::wait_counter cnt_;

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
        if (cond.open(name, &cnt_)) {
            return cond;
        }
        return invalid();
    }

    void close(handle_t& h) {
        if (h == invalid()) return;
        h.close();
    }

    template <typename F>
    bool wait_if(handle_t& h, waiter_helper::wait_flags * flags, F&& pred, std::size_t tm = invalid_value) {
        if (h == invalid()) return false;

        class non_mutex {
        public:
            void lock  () noexcept {}
            void unlock() noexcept {}
        } nm;

        return h.wait_if(nm, flags, std::forward<F>(pred), tm);
    }

    bool notify(handle_t& h) {
        if (h == invalid()) return false;
        return h.notify();
    }

    bool broadcast(handle_t& h) {
        if (h == invalid()) return false;
        return h.broadcast();
    }

    bool quit_waiting(handle_t& h, waiter_helper::wait_flags * flags) {
        if (h == invalid()) return false;
        return h.quit_waiting(flags);
    }
};

} // namespace detail
} // namespace ipc
