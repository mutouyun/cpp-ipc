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
        if (h_.acquire((name + "__COND_CNT__").c_str(), sizeof(long volatile))) {
            return ipc::detail::condition::open(name, static_cast<long volatile*>(h_.get()));
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
        if ((static_cast<info_t*>(h_.get())->opened_.fetch_add(1, std::memory_order_acq_rel) == 0) &&
            !static_cast<info_t*>(h_.get())->object_.open(std::forward<P>(params)...)) {
            return false;
        }
        return true;
    }

    void close() {
        if (!h_.valid()) return;
        if (static_cast<info_t*>(h_.get())->opened_.fetch_sub(1, std::memory_order_release) == 1) {
            static_cast<info_t*>(h_.get())->object_.close();
        }
        h_.release();
    }
};

class mutex_impl : public object_impl<ipc::detail::mutex> {
public:
    bool lock  () { return object().lock  (); }
    bool unlock() { return object().unlock(); }
};

class semaphore_impl : public object_impl<ipc::detail::semaphore> {
public:
    bool wait() {
        return object().wait_if([] { return true; });
    }

    bool post(long count) {
        return object().post([count] { return count; });
    }
};

class condition_impl : public object_impl<ipc::detail::condition> {
public:
    bool wait(mutex_impl& mtx) {
        return object().wait(mtx.object());
    }

    bool notify   () { return object().notify   (); }
    bool broadcast() { return object().broadcast(); }
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
