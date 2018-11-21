#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <atomic>
#include <utility>
#include <limits>
#include <algorithm>
#include <thread>

namespace ipc {

struct circ_queue_head {
    using u8_t = std::atomic<std::uint8_t>;
    using ms_t = std::atomic<std::size_t>; // message head

    ms_t mc_ { 0 }; // message counter
    u8_t cc_ { 0 }; // connection counter
    u8_t cr_ { 0 }; // write cursor
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
        msg_size   = elem_size - sizeof(ms_t)
    };

    static_assert(Size > head_size        , "Size must > head_size");
    static_assert(elem_size >= head_size  , "elem_size must >= head_size");
    static_assert(elem_size > sizeof(ms_t), "elem_size must > sizeof(ms_t)");
    static_assert(Size % elem_size == 0   , "Size must be multiple of elem_size");

private:
    struct elem_t {
        ms_t head_;
        std::uint8_t msg_[msg_size];
    };

    elem_t* elem_start(void) {
        return reinterpret_cast<elem_t*>(this) + 1;
    }

    static void next(u8_t& i) {
        if (++i == elem_max) i.store(0, std::memory_order_release);
    }

    std::uint8_t block_[block_size];

public:
    static void next(std::uint8_t& i) {
        if (++i == elem_max) i = 0;
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
        return cc_.fetch_add(1, std::memory_order_acq_rel);
    }

    std::size_t disconnect(void) {
        return cc_.fetch_sub(1, std::memory_order_acq_rel);
    }

    std::size_t conn_count(void) const {
        return cc_.load(std::memory_order_acquire);
    }

    void* acquire(void) {
        auto st = elem_start() + cr_.load(std::memory_order_relaxed);
        do {
            // check remain count of consumers
            if (!st->head_.load(std::memory_order_acquire)) {
                st->head_.store(conn_count());
                break;
            }
        } while(1);
        return st->msg_;
    }

    void commit(void) {
        next(cr_);
        mc_.fetch_add(1, std::memory_order_release);
    }

    std::uint8_t cursor(void) const {
        return cr_.load(std::memory_order_acquire);
    }

    constexpr std::size_t begin(void) const {
        return 0;
    }

    std::size_t end(void) const {
        return mc_.load(std::memory_order_acquire);
    }

    void* get(std::uint8_t index) {
        auto st = elem_start() + index;
        st->head_.fetch_sub(1, std::memory_order_release);
        return st->msg_;
    }
};

} // namespace ipc
