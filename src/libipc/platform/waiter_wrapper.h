#pragma once

#include <type_traits>
#include <atomic>
#include <utility>

#include "libipc/shm.h"

#include "libipc/memory/resource.h"
#include "libipc/platform/detail.h"
#if defined(IPC_OS_WINDOWS_)

#include "libipc/platform/waiter_win.h"

namespace ipc {
namespace detail {

using mutex_impl     = ipc::detail::mutex;
using semaphore_impl = ipc::detail::semaphore;

class condition_impl : public ipc::detail::condition {
    using base_t = ipc::detail::condition;

    ipc::shm::handle cnt_h_;
    waiter_helper::wait_flags flags_;

public:
    static void remove(char const * name) {
        base_t::remove(name);
        ipc::string n = name;
        ipc::shm::remove((n + "__COND_CNT__" ).c_str());
        ipc::shm::remove((n + "__COND_WAIT__").c_str());
    }

    bool open(char const * name) {
        if (cnt_h_ .acquire(
                (ipc::string { name } + "__COND_CNT__" ).c_str(), 
                sizeof(waiter_helper::wait_counter))) {
            flags_.is_closed_.store(false, std::memory_order_release);
            return base_t::open(name, 
                static_cast<waiter_helper::wait_counter *>(cnt_h_.get()));
        }
        return false;
    }

    void close() {
        flags_.is_closed_.store(true, std::memory_order_release);
        base_t::quit_waiting(&flags_);
        base_t::close();
        cnt_h_.release();
    }

    bool wait(mutex_impl& mtx, std::uint64_t tm = invalid_value) {
        return base_t::wait_if(mtx, &flags_, [] { return true; }, tm);
    }
};

} // namespace detail
} // namespace ipc

#elif defined(IPC_OS_LINUX_)

#include "libipc/platform/waiter_linux.h"

namespace ipc {
namespace detail {

template <typename T>
class object_impl {
    ipc::shm::handle h_;

    struct info_t {
        T object_;
        std::atomic<unsigned> opened_;
    };

public:
    static void remove(char const * name) {
        {
            ipc::shm::handle h { name, sizeof(info_t) };
            if (h.valid()) {
                auto info = static_cast<info_t*>(h.get());
                info->object_.close();
            }
        }
        ipc::shm::remove(name);
    }

    T& object() {
        return static_cast<info_t*>(h_.get())->object_;
    }

    template <typename... P>
    bool open(char const * name, P&&... params) {
        if (!h_.acquire(name, sizeof(info_t))) {
            return false;
        }
        auto info = static_cast<info_t*>(h_.get());
        if ((info->opened_.fetch_add(1, std::memory_order_acq_rel) == 0) &&
            !info->object_.open(std::forward<P>(params)...)) {
            return false;
        }
        return true;
    }

    void close() {
        if (!h_.valid()) return;
        auto info = static_cast<info_t*>(h_.get());
        if (info->opened_.fetch_sub(1, std::memory_order_release) == 1) {
            info->object_.close();
        }
        h_.release();
    }
};

class mutex_impl : public object_impl<ipc::detail::mutex> {
public:
    bool lock  () { return object().lock  (); }
    bool unlock() { return object().unlock(); }
};

class condition_impl : public object_impl<ipc::detail::condition> {
public:
    bool wait(mutex_impl& mtx, std::uint64_t tm = invalid_value) {
        return object().wait(mtx.object(), tm);
    }

    bool notify   () { return object().notify   (); }
    bool broadcast() { return object().broadcast(); }
};

class semaphore_impl {
    sem_helper::handle_t h_;
    ipc::shm::handle     opened_; // std::atomic<unsigned>
    ipc::string          name_;

    auto cnt() {
        return static_cast<std::atomic<unsigned>*>(opened_.get());
    }

public:
    static void remove(char const * name) {
        sem_helper::destroy((ipc::string{ "__SEMAPHORE_IMPL_SEM__" } + name).c_str());
        ipc::shm::remove   ((ipc::string{ "__SEMAPHORE_IMPL_CNT__" } + name).c_str());
    }

    bool open(char const * name, long count) {
        name_ = name;
        if (!opened_.acquire(("__SEMAPHORE_IMPL_CNT__" + name_).c_str(), sizeof(std::atomic<unsigned>))) {
            return false;
        }
        if ((h_ = sem_helper::open(("__SEMAPHORE_IMPL_SEM__" + name_).c_str(), count)) == sem_helper::invalid()) {
            return false;
        }
        cnt()->fetch_add(1, std::memory_order_acq_rel);
        return true;
    }

    void close() {
        if (h_ == sem_helper::invalid()) return;
        sem_helper::close(h_);
        if (cnt() == nullptr) return;
        if (cnt()->fetch_sub(1, std::memory_order_release) == 1) {
            sem_helper::destroy(("__SEMAPHORE_IMPL_SEM__" + name_).c_str());
        }
        opened_.release();
    }

    bool wait(std::uint64_t tm = invalid_value) {
        return sem_helper::wait(h_, tm);
    }

    bool post(long count) {
        return sem_helper::post(h_, count);
    }
};

} // namespace detail
} // namespace ipc
#else/*linux*/
#   error "Unsupported platform."
#endif

namespace ipc {
namespace detail {

class waiter_wrapper {
public:
    using waiter_t = detail::waiter;

private:
    waiter_t* w_ = nullptr;
    waiter_t::handle_t h_ = waiter_t::invalid();
    waiter_helper::wait_flags flags_;

public:
    waiter_wrapper() = default;
    explicit waiter_wrapper(waiter_t* w) {
        attach(w);
    }
    waiter_wrapper(const waiter_wrapper&) = delete;
    waiter_wrapper& operator=(const waiter_wrapper&) = delete;

    waiter_t       * waiter()       { return w_; }
    waiter_t const * waiter() const { return w_; }

    void attach(waiter_t* w) {
        close();
        w_ = w;
    }

    bool valid() const {
        return (w_ != nullptr) && (h_ != waiter_t::invalid());
    }

    bool open(char const * name) {
        if (w_ == nullptr) return false;
        close();
        flags_.is_closed_.store(false, std::memory_order_release);
        h_ = w_->open(name);
        return valid();
    }

    void close() {
        if (!valid()) return;
        flags_.is_closed_.store(true, std::memory_order_release);
        quit_waiting();
        w_->close(h_);
        h_ = waiter_t::invalid();
    }

    void quit_waiting() {
        w_->quit_waiting(h_, &flags_);
    }

    template <typename F>
    bool wait_if(F && pred, std::uint64_t tm = invalid_value) {
        if (!valid()) return false;
        return w_->wait_if(h_, &flags_, std::forward<F>(pred), tm);
    }

    bool notify() {
        if (!valid()) return false;
        w_->notify(h_);
        return true;
    }

    bool broadcast() {
        if (!valid()) return false;
        w_->broadcast(h_);
        return true;
    }
};

} // namespace detail

class waiter : public detail::waiter_wrapper {

    shm::handle shm_;

    using detail::waiter_wrapper::attach;

public:
    waiter() = default;
    waiter(char const * name) {
        open(name);
    }

    ~waiter() {
        close();
    }

    bool open(char const * name) {
        if (name == nullptr || name[0] == '\0') {
            return false;
        }
        close();
        if (!shm_.acquire((ipc::string{ "__SHM_WAITER__" } + name).c_str(), sizeof(waiter_t))) {
            return false;
        }
        attach(static_cast<waiter_t*>(shm_.get()));
        return detail::waiter_wrapper::open((ipc::string{ "__IMP_WAITER__" } + name).c_str());
    }

    void close() {
        detail::waiter_wrapper::close();
        shm_.release();
    }
};

} // namespace ipc
