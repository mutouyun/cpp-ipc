#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <atomic>
#include <utility>
#include <limits>
#include <algorithm>

namespace ipc {

struct circ_queue_head {
    using ui_t = std::atomic<std::uint16_t>;
    using el_t = std::atomic<std::size_t>; // element head

    ui_t cc_ { 0 }; // connection counter
    ui_t cr_ { 0 }; // write cursor
};

template <std::size_t Size>
class circ_queue : private circ_queue_head {
public:
    enum : std::size_t {
        total_size = Size,
        head_size  = sizeof(circ_queue_head),
        block_size = Size - head_size,
        elem_max   = std::numeric_limits<std::uint8_t>::max(),
        elem_size  = (Size / (elem_max + 1)),
        data_size  = elem_size - sizeof(el_t)
    };

    static_assert(Size > head_size        , "Size must > head_size");
    static_assert(elem_size >= head_size  , "elem_size must >= head_size");
    static_assert(elem_size > sizeof(el_t), "elem_size must > sizeof(el_t)");
    static_assert(Size % elem_size == 0   , "Size must be multiple of elem_size");

private:
    struct elem_t {
        el_t head_;
        std::uint8_t data_[data_size];
    };

    elem_t* elem_start(void) {
        return reinterpret_cast<elem_t*>(this) + 1;
    }

    static std::uint8_t id(std::uint16_t i) {
        return i & 0x00ff;
    }

    std::uint8_t block_[block_size];

public:
    static std::uint16_t next(std::uint16_t i) {
        return (id(++i) == elem_max) ? 0 : i;
    }

    circ_queue(void) {
        ::memset(block_, 0, sizeof(block_));
    }
    ~circ_queue(void) = delete;

    circ_queue(const circ_queue&) = delete;
    circ_queue& operator=(const circ_queue&) = delete;
    circ_queue(circ_queue&&) = delete;
    circ_queue& operator=(circ_queue&&) = delete;

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
        auto st = elem_start() + id(cr_.load(std::memory_order_relaxed));
        // check remain count of consumers
        do {
            std::size_t expected = 0;
            if (st->head_.compare_exchange_weak(expected, conn_count(),
                std::memory_order_consume, std::memory_order_relaxed)) {
                break;
            }
        } while(1);
        return st->data_;
    }

    void commit(void) {
        cr_.store(next(cr_.load(std::memory_order_relaxed)), std::memory_order_release);
    }

    std::uint16_t cursor(void) const {
        return cr_.load(std::memory_order_consume);
    }

    void* get(std::uint16_t index) {
        return (elem_start() + id(index))->data_;
    }

    void put(std::uint16_t index) {
        auto st = elem_start() + id(index);
        st->head_.fetch_sub(1, std::memory_order_release);
    }
};

} // namespace ipc
