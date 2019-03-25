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
#include "platform/detail.h"

namespace ipc {
namespace detail {

class queue_waiter {
protected:
    ipc::detail::waiter_wrapper waiter_;
    ipc::detail::waiter_wrapper cc_waiter_;

    bool connected_ = false;
    shm::handle elems_h_;

    template <typename Elems>
    Elems* open(char const * name) {
        if (!elems_h_.acquire(name, sizeof(Elems))) {
            return nullptr;
        }
        auto elems = static_cast<Elems*>(elems_h_.get());
        if (elems == nullptr) {
            return nullptr;
        }
        elems->init();
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
    void close(Elems* /*elems*/) {
        close();
        elems_h_.release();
    }

public:
    queue_waiter() = default;
    queue_waiter(const queue_waiter&) = delete;
    queue_waiter& operator=(const queue_waiter&) = delete;

    bool connected() const noexcept {
        return connected_;
    }

    template <typename Elems>
    auto connect(Elems* elems)
     -> std::tuple<bool, decltype(std::declval<Elems>().cursor())> {
        if (elems == nullptr) return {};
        if (connected_) {
            // if it's already connected, just return false
            return {};
        }
        connected_ = true;
        elems->connect();
        auto ret = std::make_tuple(true, elems->cursor());
        cc_waiter_.broadcast();
        return ret;
    }

    template <typename Elems>
    bool disconnect(Elems* elems) {
        if (elems == nullptr) return false;
        if (!connected_) {
            // if it's already disconnected, just return false
            return false;
        }
        connected_ = false;
        elems->disconnect();
        cc_waiter_.broadcast();
        return true;
    }

    template <typename Elems>
    bool wait_for_connect(Elems* elems, std::size_t count) {
        if (elems == nullptr) return false;
        for (unsigned k = 0; elems->conn_count() < count;) {
            ipc::sleep(k, [this, elems, count] {
                return cc_waiter_.wait_if([elems, count] {
                    return elems->conn_count() < count;
                });
            });
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
    elems_t * elems_ = nullptr;
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

    bool connect() {
        auto tp = base_t::connect(elems_);
        if (std::get<0>(tp)) {
            cursor_ = std::get<1>(tp);
            return true;
        }
        return false;
    }

    bool disconnect() {
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
        auto pop_item = [this, &item] {
            return elems_->pop(&this->cursor_, [&item](void* p) {
                ::new (&item) T(std::move(*static_cast<T*>(p)));
            });
        };
        for (unsigned k = 0;;) {
            if (pop_item()) return item;
            bool succ = false;
            ipc::sleep(k, [this, &succ, &pop_item] {
                return this->waiter_.wait_if([&succ, &pop_item] {
                    return !(succ = pop_item());
                });
            });
            if (succ) return item;
        }
    }
};

} // namespace detail

template <typename T, typename Policy>
class queue : public detail::queue_base<typename Policy::template elems_t<sizeof(T), alignof(T)>> {
    using base_t = detail::queue_base<typename Policy::template elems_t<sizeof(T), alignof(T)>>;

public:
    using base_t::base_t;
    using base_t::push;
    using base_t::pop;

    template <typename... P>
    auto push(P&&... params) {
        return base_t::template push<T>(std::forward<P>(params)...);
    }

    T pop() {
        return base_t::template pop<T>();
    }
};

} // namespace ipc
