#pragma once

#include <type_traits>
#include <new>
#include <utility>
#include <algorithm>
#include <atomic>
#include <tuple>
#include <thread>
#include <chrono>
#include <string>

#include "def.h"
#include "rw_lock.h"
#include "elem_circ.h"

#include "platform/waiter.h"

namespace ipc {
namespace detail {

class queue_waiter {
protected:
    ipc::detail::waiter_impl waiter_, cc_waiter_;
    std::atomic_bool connected_ { false };

    template <typename Elems>
    void open(Elems* elems, char const * name) {
        waiter_.attach(&(elems->waiter()));
        waiter_.open((std::string{ "__IPC_WAITER__" } + name).c_str());
        cc_waiter_.attach(&(elems->conn_waiter()));
        cc_waiter_.open((std::string{ "__IPC_CC_WAITER__" } + name).c_str());
    }

    void close() {
        waiter_.close();
        waiter_.attach(nullptr);
        cc_waiter_.close();
        cc_waiter_.attach(nullptr);
    }

public:
    queue_waiter() = default;
    queue_waiter(const queue_waiter&) = delete;
    queue_waiter& operator=(const queue_waiter&) = delete;

    bool connected() const noexcept {
        return connected_.load(std::memory_order_acquire);
    }

    template <typename Elems>
    std::size_t connect(Elems* elems) {
        if (elems == nullptr) return invalid_value;
        if (connected_.exchange(true, std::memory_order_acq_rel)) {
            // if it's already connected, just return an error count
            return invalid_value;
        }
        auto ret = elems->connect();
        cc_waiter_.broadcast();
        return ret;
    }

    template <typename Elems>
    std::size_t disconnect(Elems* elems) {
        if (elems == nullptr) return invalid_value;
        if (!connected_.exchange(false, std::memory_order_acq_rel)) {
            // if it's already disconnected, just return an error count
            return invalid_value;
        }
        auto ret = elems->disconnect();
        cc_waiter_.broadcast();
        return ret;
    }

    template <typename Elems>
    bool wait_for_connect(Elems* elems, std::size_t count) {
        if (elems == nullptr) return false;
        for (unsigned k = 0; elems->conn_count() < count;) {
            ipc::sleep(k, [this] { return cc_waiter_.wait(); });
        }
        return true;
    }
};

template <typename Elems>
class queue_base : public queue_waiter {
    using base_t = queue_waiter;

public:
    using elems_t  = Elems;
    using policy_t = typename elems_t::policy_t;

protected:
    elems_t* elems_ = nullptr;
    decltype(std::declval<elems_t>().cursor()) cursor_ = 0;

public:
    using base_t::base_t;

    explicit queue_base(elems_t* els, char const * name = nullptr)
        : queue_base() {
        attach(els, name);
    }

    constexpr elems_t * elems() const noexcept {
        return elems_;
    }

    std::size_t connect() {
        return base_t::connect(elems_);
    }

    std::size_t disconnect() {
        return base_t::disconnect(elems_);
    }

    std::size_t conn_count() const noexcept {
        return (elems_ == nullptr) ? invalid_value : elems_->conn_count();
    }

    bool wait_for_connect(std::size_t count) {
        return base_t::wait_for_connect(elems_, count);
    }

    bool empty() const noexcept {
        return (elems_ == nullptr) ? true : (cursor_ == elems_->cursor());
    }

    elems_t* attach(elems_t* els, char const * name = nullptr) noexcept {
        if (els == nullptr) return nullptr;
        auto old = elems_;
        elems_ = els;
        if (name == nullptr) {
            base_t::close();
        }
        else base_t::open(elems_, name);
        cursor_ = elems_->cursor();
        return old;
    }

    elems_t* detach() noexcept {
        if (elems_ == nullptr) return nullptr;
        auto old = elems_;
        elems_ = nullptr;
        return old;
    }
};

template <typename Elems, typename IsFixed>
class queue : public queue_base<Elems> {
    using base_t = queue_base<Elems>;

public:
    using is_fixed = IsFixed;

    using base_t::base_t;

    template <typename T, typename... P>
    auto push(P&&... params) {
        if (this->elems_ == nullptr) return false;
        if (this->elems_->push([&](void* p) {
            ::new (p) T(std::forward<P>(params)...);
        })) {
            this->waiter_.broadcast();
            return true;
        }
        return false;
    }

    template <typename T>
    T pop() {
        if (this->elems_ == nullptr) {
            return {};
        }
        T item;
        for (unsigned k = 0;;) {
            if (this->elems_->pop(&this->cursor_, [&item](void* p) {
                ::new (&item) T(std::move(*static_cast<T*>(p)));
            })) {
                return item;
            }
            ipc::sleep(k, [this] { return this->waiter_.wait(); });
        }
    }
};

} // namespace detail

template <typename T,
          typename Policy = ipc::circ::prod_cons<relat::single, relat::multi, trans::broadcast>>
class queue : public detail::queue<typename Policy::template elems_t<sizeof(T)>, typename Policy::is_fixed> {
    using base_t = detail::queue<typename Policy::template elems_t<sizeof(T)>, typename Policy::is_fixed>;

public:
    using base_t::base_t;

    template <typename... P>
    auto push(P&&... params) {
        return base_t::push<T>(std::forward<P>(params)...);
    }

    T pop() {
        return base_t::pop<T>();
    }
};

} // namespace ipc
