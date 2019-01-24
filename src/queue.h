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
#include <cassert>

#include "def.h"
#include "shm.h"
#include "rw_lock.h"

#include "platform/waiter_wrapper.h"

namespace ipc {
namespace detail {

class queue_waiter {
protected:
    ipc::detail::waiter_wrapper waiter_;
    ipc::detail::waiter_wrapper cc_waiter_;

    bool connected_ = false;
    bool dismiss_   = true;

    template <typename Elems>
    Elems* open(char const * name) {
        auto elems = static_cast<Elems*>(shm::acquire(name, sizeof(Elems)));
        if (elems == nullptr) {
            return nullptr;
        }
        elems->init();
        dismiss_ = false;
        return elems;
    }

    template <typename Elems>
    void open(Elems*(& elems), char const * name) {
        assert(name != nullptr && name[0] != '\0');
        if (elems == nullptr) {
            elems = open<Elems>(name);
        }
        assert(elems != nullptr);
        waiter_.attach(&(elems->waiter()));
        if (!waiter_.open((std::string{ "__IPC_WAITER__" } + name).c_str())) {
            return;
        }
        cc_waiter_.attach(&(elems->conn_waiter()));
        cc_waiter_.open((std::string{ "__IPC_CC_WAITER__" } + name).c_str());
    }

    void close() {
        waiter_.close();
        waiter_.attach(nullptr);
        cc_waiter_.close();
        cc_waiter_.attach(nullptr);
    }

    template <typename Elems>
    void close(Elems* elems) {
        close();
        if (!dismiss_ && (elems != nullptr)) {
            shm::release(elems, sizeof(Elems));
        }
        dismiss_ = true;
    }

public:
    queue_waiter() = default;
    queue_waiter(const queue_waiter&) = delete;
    queue_waiter& operator=(const queue_waiter&) = delete;

    bool connected() const noexcept {
        return connected_;
    }

    template <typename Elems>
    std::size_t connect(Elems* elems) {
        if (elems == nullptr) return invalid_value;
        if (connected_) {
            // if it's already connected, just return an error count
            return invalid_value;
        }
        connected_ = true;
        auto ret = elems->connect();
        cc_waiter_.broadcast();
        return ret;
    }

    template <typename Elems>
    std::size_t disconnect(Elems* elems) {
        if (elems == nullptr) return invalid_value;
        if (!connected_) {
            // if it's already disconnected, just return an error count
            return invalid_value;
        }
        connected_ = false;
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

    queue_base() = default;

    explicit queue_base(char const * name)
        : queue_base() {
        attach(nullptr, name);
    }

    explicit queue_base(elems_t* els, char const * name = nullptr)
        : queue_base() {
        attach(els, name);
    }

    /* not virtual */ ~queue_base(void) {
        base_t::close(elems_);
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

    bool valid() const noexcept {
        return elems_ != nullptr;
    }

    bool empty() const noexcept {
        return (elems_ == nullptr) ? true : (cursor_ == elems_->cursor());
    }

    elems_t* attach(elems_t* els, char const * name = nullptr) noexcept {
        auto old = elems_;
        elems_ = els;
        if (name == nullptr || name[0] == '\0') {
            base_t::close(old);
        }
        else base_t::open(elems_, name);
        if (elems_ != nullptr) {
            cursor_ = elems_->cursor();
        }
        return old;
    }

    elems_t* detach() noexcept {
        if (elems_ == nullptr) return nullptr;
        base_t::close<elems_t>(nullptr); // not release shm
        auto old = elems_;
        elems_ = nullptr;
        return old;
    }

    template <typename T, typename... P>
    auto push(P&&... params) {
        if (elems_ == nullptr) return false;
        if (elems_->push([&](void* p) {
            ::new (p) T(std::forward<P>(params)...);
        })) {
            this->waiter_.broadcast();
            return true;
        }
        return false;
    }

    template <typename T>
    T pop() {
        if (elems_ == nullptr) {
            return {};
        }
        T item;
        for (unsigned k = 0;;) {
            if (elems_->pop(&this->cursor_, [&item](void* p) {
                ::new (&item) T(std::move(*static_cast<T*>(p)));
            })) {
                return item;
            }
            ipc::sleep(k, [this] { return this->waiter_.wait(); });
        }
    }
};

} // namespace detail

template <typename T, typename Policy>
class queue : public detail::queue_base<typename Policy::template elems_t<sizeof(T)>> {
    using base_t = detail::queue_base<typename Policy::template elems_t<sizeof(T)>>;

public:
    using base_t::base_t;
    using base_t::push;
    using base_t::pop;

    template <typename... P>
    auto push(P&&... params) {
        return base_t::template push<T>(std::forward<P>(params)...);
    }

    T pop() { return base_t::template pop<T>(); }
};

} // namespace ipc
