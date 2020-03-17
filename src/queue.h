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
#include "shm.h"
#include "log.h"
#include "rw_lock.h"

#include "platform/detail.h"
#include "circ/elem_def.h"

namespace ipc {
namespace detail {

class queue_conn {
protected:
    circ::cc_t connected_ = 0;
    shm::handle elems_h_;

    template <typename Elems>
    Elems* open(char const * name) {
        if (name == nullptr || name[0] == '\0') {
            ipc::error("fail open waiter: name is empty!\n");
            return nullptr;
        }
        if (!elems_h_.acquire(name, sizeof(Elems))) {
            return nullptr;
        }
        auto elems = static_cast<Elems*>(elems_h_.get());
        if (elems == nullptr) {
            ipc::error("fail acquire elems: %s\n", name);
            return nullptr;
        }
        elems->init();
        return elems;
    }

    void close() {
        elems_h_.release();
    }

public:
    queue_conn() = default;
    queue_conn(const queue_conn&) = delete;
    queue_conn& operator=(const queue_conn&) = delete;

    bool connected() const noexcept {
        return connected_ != 0;
    }

    circ::cc_t connected_id() const noexcept {
        return connected_;
    }

    template <typename Elems>
    auto connect(Elems* elems)
     -> std::tuple<bool, decltype(std::declval<Elems>().cursor())> {
        if (elems == nullptr) return {};
        // if it's already connected, just return false
        if (connected()) return {};
        connected_ = elems->connect();
        return std::make_tuple(connected(), elems->cursor());
    }

    template <typename Elems>
    bool disconnect(Elems* elems) {
        if (elems == nullptr) return false;
        // if it's already disconnected, just return false
        if (!connected()) return false;
        elems->disconnect(connected_);
        return true;
    }
};

template <typename Elems>
class queue_base : public queue_conn {
    using base_t = queue_conn;

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
        elems_ = open<elems_t>(name);
    }

    /* not virtual */ ~queue_base() {
        base_t::close();
    }

    constexpr elems_t       * elems()       noexcept { return elems_; }
    constexpr elems_t const * elems() const noexcept { return elems_; }

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

    bool valid() const noexcept {
        return elems_ != nullptr;
    }

    bool empty() const noexcept {
        return !valid() || (cursor_ == elems_->cursor());
    }

    template <typename T, typename... P>
    auto push(P&&... params) {
        if (elems_ == nullptr) return false;
        return elems_->push(this, [&](void* p) {
            ::new (p) T(std::forward<P>(params)...);
        });
    }

    template <typename T, typename F, typename... P>
    auto force_push(F&& prep, P&&... params) {
        if (elems_ == nullptr) return false;
        return elems_->force_push(this, [&](void* p) {
            if (prep(p)) ::new (p) T(std::forward<P>(params)...);
        });
    }

    template <typename T>
    bool pop(T& item) {
        if (elems_ == nullptr) {
            return false;
        }
        return elems_->pop(this, &(this->cursor_), [&item](void* p) {
            ::new (&item) T(std::move(*static_cast<T*>(p)));
        });
    }
};

} // namespace detail

template <typename T, typename Policy>
class queue : public detail::queue_base<typename Policy::template elems_t<sizeof(T), alignof(T)>> {
    using base_t = detail::queue_base<typename Policy::template elems_t<sizeof(T), alignof(T)>>;

public:
    using value_t = T;

    using base_t::base_t;

    template <typename... P>
    auto push(P&&... params) {
        return base_t::template push<T>(std::forward<P>(params)...);
    }

    template <typename... P>
    auto force_push(P&&... params) {
        return base_t::template force_push<T>(std::forward<P>(params)...);
    }

    bool pop(T& item) {
        return base_t::pop(item);
    }
};

} // namespace ipc
