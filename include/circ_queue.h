#pragma once

#include <type_traits>
#include <new>
#include <utility>
#include <algorithm>
#include <atomic>
#include <tuple>
#include <thread>
#include <chrono>

#include "def.h"
#include "circ_elem_array.h"

namespace ipc {
namespace circ {

template <typename T, typename Policy = prod_cons<relat::single, relat::multi, trans::broadcast>>
class queue {
public:
    using array_t  = elem_array<sizeof(T), Policy>;
    using policy_t = typename array_t::policy_t;

private:
    array_t* elems_ = nullptr;
    decltype(std::declval<array_t>().cursor()) cursor_ = 0;
    std::atomic_bool connected_ { false };

public:
    queue() = default;

    explicit queue(array_t* arr) : queue() {
        attach(arr);
    }

    queue(const queue&) = delete;
    queue& operator=(const queue&) = delete;
    queue(queue&&) = delete;
    queue& operator=(queue&&) = delete;

    constexpr array_t * elems() const noexcept {
        return elems_;
    }

    std::size_t connect() noexcept {
        if (elems_ == nullptr) return invalid_value;
        if (connected_.exchange(true, std::memory_order_acq_rel)) {
            // if it's already connected, just return an error count
            return invalid_value;
        }
        return elems_->connect();
    }

    std::size_t disconnect() noexcept {
        if (elems_ == nullptr) return invalid_value;
        if (!connected_.exchange(false, std::memory_order_acq_rel)) {
            // if it's already disconnected, just return an error count
            return invalid_value;
        }
        return elems_->disconnect();
    }

    std::size_t conn_count() const noexcept {
        return (elems_ == nullptr) ? invalid_value : elems_->conn_count();
    }

    bool empty() const noexcept {
        return (elems_ == nullptr) ? true : (cursor_ == elems_->cursor());
    }

    bool connected() const noexcept {
        return connected_.load(std::memory_order_acquire);
    }

    array_t* attach(array_t* arr) noexcept {
        if (arr == nullptr) return nullptr;
        auto old = elems_;
        elems_  = arr;
        cursor_ = elems_->cursor();
        return old;
    }

    array_t* detach() noexcept {
        if (elems_ == nullptr) return nullptr;
        auto old = elems_;
        elems_ = nullptr;
        return old;
    }

    template <typename... P>
    auto push(P&&... params) noexcept {
        if (elems_ == nullptr) return false;
        return elems_->push([&](void* p) {
            ::new (p) T(std::forward<P>(params)...);
        });
    }

    template <typename F>
    static queue* multi_wait_for(F&& upd) noexcept {
        for (uint_t<32> k = 0;;) {
            auto [ques, size] = upd();
            for (std::size_t i = 0; i < static_cast<std::size_t>(size); ++i) {
                queue* que = ques[i];
                if ((que != nullptr) && (que->elems_ == nullptr ||
                                         que->elems_->cursor() != que->cursor_)) {
                    // may que->elems_ == nullptr
                    return que;
                }
            }
            if (k < 4096) {
                std::this_thread::yield();
                ++k;
            }
            else std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    static T pop(queue* que) noexcept {
        if (que == nullptr) {
            return {};
        }
        if (que->elems_ == nullptr) {
            return {};
        }
        T item;
        if (!que->elems_->pop(que->cursor_, [&item](void* p) {
            ::new (&item) T(std::move(*static_cast<T*>(p)));
        })) {
            return {};
        }
        return item;
    }

    T pop() noexcept {
        return pop(multi_wait_for([que = this] {
            return std::make_tuple(&que, 1);
        }));
    }
};

} // namespace circ
} // namespace ipc
