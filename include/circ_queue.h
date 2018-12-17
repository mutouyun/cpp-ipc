#pragma once

#include <type_traits>
#include <new>
#include <exception>
#include <utility>
#include <algorithm>

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
    bool connected_ = false;

public:
    queue() = default;

    explicit queue(array_t* arr) : queue() {
        attach(arr);
    }

    queue(queue&& rhs) : queue() {
        swap(rhs);
    }

    void swap(queue& rhs) {
        std::swap(elems_    , rhs.elems_    );
        std::swap(cursor_   , rhs.cursor_   );
        std::swap(connected_, rhs.connected_);
    }

    queue& operator=(queue rhs) {
        swap(rhs);
        return *this;
    }

    array_t * elems() {
        return elems_;
    }

    std::size_t connect() {
        if (elems_ == nullptr) return error_count;
        if (connected_) return error_count;
        connected_ = true;
        return elems_->connect();
    }

    std::size_t disconnect() {
        if (elems_ == nullptr) return error_count;
        if (!connected_) return error_count;
        connected_ = false;
        return elems_->disconnect();
    }

    std::size_t conn_count() const {
        return (elems_ == nullptr) ? error_count : elems_->conn_count();
    }

    bool connected() const {
        return connected_;
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
     -> std::enable_if_t<!std::is_same<std::remove_reference_t<P>, T>::value, bool> {
        if (elems_ == nullptr) return false;
        auto ptr = elems_->acquire();
        ::new (ptr) T { std::forward<P>(param) };
        elems_->commit(ptr);
        return true;
    }

    template <typename... P>
    auto push(P&&... params) // some old compilers are not support this well
     -> std::enable_if_t<(sizeof...(P) != 1), bool> {
        if (elems_ == nullptr) return false;
        auto ptr = elems_->acquire();
        ::new (ptr) T { std::forward<P>(params)... };
        elems_->commit(ptr);
        return true;
    }

    T pop() {
        if (elems_ == nullptr) throw std::invalid_argument {
            "This queue hasn't attached any elem_array."
        };
        while (cursor_ == elems_->cursor()) {
            std::this_thread::yield();
        }
        auto item_ptr = static_cast<T*>(elems_->take(cursor_++));
        T item = *item_ptr;
        elems_->put(item_ptr);
        return item;
    }
};

} // namespace circ
} // namespace ipc
