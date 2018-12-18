#pragma once

#include <type_traits>
#include <new>
#include <exception>
#include <utility>
#include <algorithm>
#include <atomic>

#include "def.h"
#include "circ_elem_array.h"

namespace ipc {
namespace circ {

template <typename T>
class queue {
public:
    using array_t = elem_array<sizeof(T)>;

private:
    array_t* elems_ = nullptr;
    typename array_t::u2_t cursor_ = 0;
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

    constexpr array_t * elems() const {
        return elems_;
    }

    std::size_t connect() {
        if (elems_ == nullptr) return error_count;
        if (connected_.exchange(true, std::memory_order_relaxed)) {
            // if it's already connected, just return an error count
            return error_count;
        }
        return elems_->connect();
    }

    std::size_t disconnect() {
        if (elems_ == nullptr) return error_count;
        if (!connected_.exchange(false, std::memory_order_relaxed)) {
            // if it's already disconnected, just return an error count
            return error_count;
        }
        return elems_->disconnect();
    }

    std::size_t conn_count() const {
        return (elems_ == nullptr) ? error_count : elems_->conn_count();
    }

    bool connected() const {
        return connected_.load(std::memory_order_relaxed);
    }

    array_t* attach(array_t* arr) {
        if (arr == nullptr) return nullptr;
        auto old = elems_;
        elems_  = arr;
        cursor_ = elems_->cursor();
        return old;
    }

    array_t* detach() {
        if (elems_ == nullptr) return nullptr;
        auto old = elems_;
        elems_ = nullptr;
        return old;
    }

    bool push(T const & item) {
        if (elems_ == nullptr) return false;
        auto ptr = elems_->acquire();
        ::new (ptr) T(item);
        elems_->commit(ptr);
        return true;
    }

    template <typename P>
    auto push(P&& param) // disable this if P is the same as T
     -> Requires<!std::is_same<std::remove_reference_t<P>, T>::value, bool> {
        if (elems_ == nullptr) return false;
        auto ptr = elems_->acquire();
        ::new (ptr) T { std::forward<P>(param) };
        elems_->commit(ptr);
        return true;
    }

    template <typename... P>
    auto push(P&&... params) // some old compilers are not support this well
     -> Requires<(sizeof...(P) != 1), bool> {
        if (elems_ == nullptr) return false;
        auto ptr = elems_->acquire();
        ::new (ptr) T { std::forward<P>(params)... };
        elems_->commit(ptr);
        return true;
    }

    template <typename QArr, typename At>
    static T multi_pop(QArr& ques, std::size_t size, At&& at) {
        if (size == 0) throw std::invalid_argument { "Invalid size." };
        while (1) {
            for (std::size_t i = 0; i < size; ++i) {
                queue* cq = at(ques, i);
                if (cq->elems_ == nullptr) throw std::logic_error {
                    "This queue hasn't attached any elem_array."
                };
                if (cq->cursor_ != cq->elems_->cursor()) {
                    auto item_ptr = static_cast<T*>(cq->elems_->take(cq->cursor_++));
                    T item = std::move(*item_ptr);
                    cq->elems_->put(item_ptr);
                    return item;
                }
            }
            std::this_thread::yield();
        }
    }

    static T multi_pop(queue* ques, std::size_t size) {
        if (ques == nullptr) throw std::invalid_argument { "Invalid ques pointer." };
        return multi_pop(ques, size, [](queue* ques, std::size_t i) {
            return ques + i;
        });
    }

    static T multi_pop(std::vector<queue*>& ques) {
        return multi_pop(ques, ques.size(), [](auto& ques, std::size_t i) {
            return ques[i];
        });
    }

    T pop() { return multi_pop(this, 1); }
};

} // namespace circ
} // namespace ipc
