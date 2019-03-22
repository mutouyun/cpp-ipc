#pragma once

#include <type_traits>
#include <atomic>
#include <utility>

#include "shm.h"

#include "platform/detail.h"
#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || \
    defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || \
    defined(WINCE) || defined(_WIN32_WCE)

#include "platform/waiter_win.h"

namespace ipc {
namespace detail {

using mutex_impl     = ipc::detail::mutex;
using semaphore_impl = ipc::detail::semaphore;

class condition_impl : public ipc::detail::condition {
    ipc::shm::handle h_;

public:
    bool open(std::string const & name) {
        if (h_.acquire((name + "__COND_CNT__").c_str(), sizeof(long))) {
            return ipc::detail::condition::open(name, static_cast<long *>(h_.get()));
        }
        return false;
    }

    void close() {
        ipc::detail::condition::close();
        h_.release();
    }

    bool wait(mutex_impl& mtx) {
        return ipc::detail::condition::wait_if(mtx, [] { return true; });
    }
};

} // namespace detail
} // namespace ipc

#else /*!WIN*/

#include "platform/waiter_linux.h"

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
    bool wait(mutex_impl& mtx) {
        return object().wait(mtx.object());
    }

    bool notify   () { return object().notify   (); }
    bool broadcast() { return object().broadcast(); }
};

class semaphore_impl {
    sem_helper::handle_t h_;
    ipc::shm::handle     opened_; // std::atomic<unsigned>
    std::string          name_;

    auto cnt() {
        return static_cast<std::atomic<unsigned>*>(opened_.get());
    }

public:
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

    bool wait() {
        if (h_ == sem_helper::invalid()) return false;
        return sem_helper::wait(h_);
    }

    bool post(long count) {
        if (h_ == sem_helper::invalid()) return false;
        bool ret = true;
        for (long i = 0; i < count; ++i) {
            ret = ret && sem_helper::post(h_);
        }
        return ret;
    }
};

} // namespace detail
} // namespace ipc

#endif/*!WIN*/

namespace ipc {
namespace detail {

class waiter_wrapper {
public:
    using waiter_t = detail::waiter;

private:
    waiter_t* w_ = nullptr;
    waiter_t::handle_t h_ = waiter_t::invalid();

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
        h_ = w_->open(name);
        return valid();
    }

    void close() {
        if (!valid()) return;
        w_->close(h_);
        h_ = waiter_t::invalid();
    }

    template <typename F>
    bool wait_if(F&& pred) {
        if (!valid()) return false;
        return w_->wait_if(h_, std::forward<F>(pred));
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
} // namespace ipc
