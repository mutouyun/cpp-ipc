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

template <typename T,
          typename Policy = ipc::circ::prod_cons<relat::single, relat::multi, trans::broadcast>>
class queue {
public:
    using elems_t  = typename Policy::template elems_t<sizeof(T)>;
    using policy_t = typename elems_t::policy_t;

private:
    elems_t* elems_ = nullptr;
    ipc::detail::waiter_impl waiter_, cc_waiter_;
    decltype(std::declval<elems_t>().cursor()) cursor_ = 0;
    std::atomic_bool connected_ { false };

public:
    queue() = default;

    explicit queue(elems_t* els, char const * name = nullptr) : queue() {
        attach(els, name);
    }

    queue(const queue&) = delete;
    queue& operator=(const queue&) = delete;

    constexpr elems_t * elems() const noexcept {
        return elems_;
    }

    std::size_t connect() {
        if (elems_ == nullptr) return invalid_value;
        if (connected_.exchange(true, std::memory_order_acq_rel)) {
            // if it's already connected, just return an error count
            return invalid_value;
        }
        auto ret = elems_->connect();
        cc_waiter_.broadcast();
        return ret;
    }

    std::size_t disconnect() {
        if (elems_ == nullptr) return invalid_value;
        if (!connected_.exchange(false, std::memory_order_acq_rel)) {
            // if it's already disconnected, just return an error count
            return invalid_value;
        }
        auto ret = elems_->disconnect();
        cc_waiter_.broadcast();
        return ret;
    }

    std::size_t conn_count() const noexcept {
        return (elems_ == nullptr) ? invalid_value : elems_->conn_count();
    }

    bool wait_for_connect(std::size_t count) {
        if (elems_ == nullptr) return false;
        for (unsigned k = 0; elems_->conn_count() < count;) {
            ipc::sleep(k, [this] { return cc_waiter_.wait(); });
        }
        return true;
    }

    bool empty() const noexcept {
        return (elems_ == nullptr) ? true : (cursor_ == elems_->cursor());
    }

    bool connected() const noexcept {
        return connected_.load(std::memory_order_acquire);
    }

    elems_t* attach(elems_t* els, char const * name = nullptr) noexcept {
        if (els == nullptr) return nullptr;
        auto old = elems_;
        elems_  = els;
        if (name == nullptr) {
            waiter_.close();
            waiter_.attach(nullptr);
            cc_waiter_.close();
            cc_waiter_.attach(nullptr);
        }
        else {
            waiter_.attach(&(elems_->waiter()));
            waiter_.open((std::string{ "__IPC_WAITER__" } + name).c_str());
            cc_waiter_.attach(&(elems_->conn_waiter()));
            cc_waiter_.open((std::string{ "__IPC_CC_WAITER__" } + name).c_str());
        }
        cursor_ = elems_->cursor();
        return old;
    }

    elems_t* detach() noexcept {
        if (elems_ == nullptr) return nullptr;
        auto old = elems_;
        elems_ = nullptr;
        return old;
    }

    template <typename... P>
    auto push(P&&... params) {
        if (elems_ == nullptr) return false;
        if (elems_->push([&](void* p) {
            ::new (p) T(std::forward<P>(params)...);
        })) {
            waiter_.broadcast();
            return true;
        }
        return false;
    }

    T pop() {
        if (elems_ == nullptr) {
            return {};
        }
        T item;
        for (unsigned k = 0;;) {
            if (elems_->pop(&cursor_, [&item](void* p) {
                ::new (&item) T(std::move(*static_cast<T*>(p)));
            })) {
                return item;
            }
            ipc::sleep(k, [this] { return waiter_.wait(); });
        }
    }
};

} // namespace ipc
