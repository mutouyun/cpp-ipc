#pragma once

#include <type_traits>
#include <new>
#include <exception>
#include <utility>

#include "circ_elem_array.h"

namespace ipc {

enum : std::size_t {
    error_count = std::numeric_limits<std::size_t>::max()
};

namespace circ {

template <typename T>
class queue {
public:
    using array_t = elem_array<sizeof(T)>;

private:
    array_t * elems_ = nullptr;
    typename std::result_of<decltype(&array_t::cursor)(array_t)>::type cursor_ = 0;

public:
    std::size_t connect(array_t* arr) {
        if (arr == nullptr) return error_count;
        elems_  = arr;
        cursor_ = elems_->cursor();
        return elems_->connect();
    }

    std::size_t disconnect(void) {
        if (elems_ == nullptr) return error_count;
        auto ret = elems_->disconnect();
        elems_ = nullptr;
        return ret;
    }

    std::size_t conn_count(void) const {
        return (elems_ == nullptr) ? error_count : elems_->conn_count();
    }

    template <typename... P>
    void push(P&&... params) {
        if (elems_ == nullptr) return;
        auto ptr = elems_->acquire();
        ::new (ptr) T { std::forward<P>(params)... };
        elems_->commit(ptr);
    }

    T pop(void) {
        if (elems_ == nullptr) throw std::invalid_argument {
            "This queue hasn't connected to any elem_array."
        };
        while (cursor_ == elems_->cursor()) {
            std::this_thread::yield();
        }
        auto item_ptr = static_cast<T*>(elems_->take(cursor_));
        T item { *item_ptr };
        elems_->put(item_ptr);
        return item;
    }
};

} // namespace circ
} // namespace ipc
