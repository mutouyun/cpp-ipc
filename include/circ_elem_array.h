#pragma once

#include <cstring>
#include <atomic>
#include <utility>
#include <algorithm>
#include <thread>

#include "def.h"

namespace ipc {
namespace circ {

struct alignas(std::max_align_t) elem_array_head {
    using ui_t = std::uint8_t;
    using uc_t = std::uint16_t;
    using ac_t = std::atomic<uc_t>;

    std::atomic<std::size_t> lc_ { 0 }; // write spin lock flag

    ac_t cc_ { 0 }; // connection counter, using for broadcast
    ac_t wt_ { 0 }; // write index
};

enum : std::size_t {
    elem_array_head_size =
        (sizeof(elem_array_head) % alignof(std::max_align_t)) ?
       ((sizeof(elem_array_head) / alignof(std::max_align_t)) + 1) * alignof(std::max_align_t) :
         sizeof(elem_array_head)
};

template <std::size_t DataSize>
class elem_array : private elem_array_head {
    struct head_t {
        std::atomic<std::uint32_t> rc_ { 0 }; // read counter
    };

public:
    enum : std::size_t {
        head_size  = elem_array_head_size,
        data_size  = DataSize,
        elem_max   = std::numeric_limits<ui_t>::max() + 1, // default is 255 + 1
        elem_size  = sizeof(head_t) + DataSize,
        block_size = elem_size * elem_max
    };

private:
    struct elem_t {
        head_t head_;
        byte_t data_[data_size];
        elem_t(void) { ::memset(data_, 0, sizeof(data_)); }
    };
    elem_t block_[elem_max];

    elem_t* elem_start(void) {
        return block_;
    }

    static elem_t* elem(void* ptr) {
        return reinterpret_cast<elem_t*>(static_cast<byte_t*>(ptr) - sizeof(head_t));
    }

    elem_t* elem(ui_t i) {
        return elem_start() + i;
    }

    static ui_t index_of(uc_t c) {
        return static_cast<ui_t>(c);
    }

    ui_t index_of(elem_t* el) {
        return static_cast<ui_t>(el - elem_start());
    }

public:
    elem_array(void) = default;

    elem_array(const elem_array&) = delete;
    elem_array& operator=(const elem_array&) = delete;
    elem_array(elem_array&&) = delete;
    elem_array& operator=(elem_array&&) = delete;

    std::size_t connect(void) {
        return cc_.fetch_add(1, std::memory_order_release);
    }

    std::size_t disconnect(void) {
        return cc_.fetch_sub(1, std::memory_order_release);
    }

    std::size_t conn_count(void) const {
        return cc_.load(std::memory_order_consume);
    }

    void* acquire(void) {
        while (lc_.exchange(1, std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        elem_t* el = elem(index_of(wt_.load(std::memory_order_relaxed)));
        // check all consumers have finished reading
        while(1) {
            std::uint32_t expected = 0;
            if (el->head_.rc_.compare_exchange_weak(
                        expected,
                        static_cast<std::uint32_t>(cc_.load(std::memory_order_relaxed)),
                        std::memory_order_release)) {
                break;
            }
            std::this_thread::yield();
            std::atomic_thread_fence(std::memory_order_acquire);
        }
        return el->data_;
    }

    void commit(void* /*ptr*/) {
        wt_.fetch_add(1, std::memory_order_relaxed);
        lc_.store(0, std::memory_order_release);
    }

    uc_t cursor(void) const {
        return wt_.load(std::memory_order_consume);
    }

    void* take(uc_t cursor) {
        return elem(index_of(cursor))->data_;
    }

    void put(void* ptr) {
        elem(ptr)->head_.rc_.fetch_sub(1, std::memory_order_release);
    }
};

} // namespace circ
} // namespace ipc
