#pragma once

#include <cstddef>
#include <cstdint>
#include <atomic>
#include <utility>
#include <limits>
#include <algorithm>
#include <tuple>

namespace ipc {

struct circ_queue_head {
    using u8_t = std::atomic<std::uint8_t>;

    u8_t cc_; // connection count
    u8_t rd_; // read-index
    u8_t wt_; // write-index
};

template <std::size_t Size>
class circ_queue : public circ_queue_head {
public:
    enum : std::size_t {
        total_size = Size,
        head_size  = sizeof(circ_queue_head),
        block_size = Size - head_size,
        elem_max   = std::numeric_limits<std::uint8_t>::max(),
        elem_size  = Size / (elem_max + 1)
    };

    static_assert(Size > head_size      , "Size must > head_size");
    static_assert(elem_size >= head_size, "elem_size must >= head_size");
    static_assert(Size % elem_size == 0 , "Size must be multiple of elem_size");

    circ_queue(void) = default;
    ~circ_queue(void) = delete;

    circ_queue(const circ_queue&) = delete;
    circ_queue& operator=(const circ_queue&) = delete;
    circ_queue(circ_queue&&) = delete;
    circ_queue& operator=(circ_queue&&) = delete;

    std::size_t connect(void) {
        return cc_.fetch_add(1, std::memory_order_relaxed);
    }

    std::size_t disconnect(void) {
        return cc_.fetch_sub(1, std::memory_order_relaxed);
    }

    std::tuple<void*, std::uint8_t> acquire(void) {
        std::uint8_t wt_id = wt_.fetch_add(1, std::memory_order_relaxed);
        return std::make_tuple(&(block_[wt_id]), wt_id);
    }

    void commit() {

    }

private:
    std::uint8_t block_[block_size];
};

} // namespace ipc
