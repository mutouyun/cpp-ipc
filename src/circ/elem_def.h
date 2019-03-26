#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <new>

#include "rw_lock.h"

#include "platform/detail.h"

namespace ipc {
namespace circ {

enum {
    cache_line_size = 64
};

using u1_t = ipc::uint_t<8>;
using u2_t = ipc::uint_t<32>;

constexpr u1_t index_of(u2_t c) noexcept {
    return static_cast<u1_t>(c);
}

class conn_head {
    std::atomic<std::size_t> cc_ { 0 }; // connection counter

    ipc::spin_lock lc_;
    std::atomic<bool> constructed_;

public:
    void init() {
        /* DCLP */
        if (!constructed_.load(std::memory_order_acquire)) {
            IPC_UNUSED_ auto guard = ipc::detail::unique_lock(lc_);
            if (!constructed_.load(std::memory_order_relaxed)) {
                ::new (this) conn_head;
                constructed_.store(true, std::memory_order_release);
            }
        }
    }

    conn_head() = default;
    conn_head(const conn_head&) = delete;
    conn_head& operator=(const conn_head&) = delete;

    std::size_t connect() noexcept {
        return cc_.fetch_add(1, std::memory_order_acq_rel);
    }

    std::size_t disconnect() noexcept {
        return cc_.fetch_sub(1, std::memory_order_acq_rel);
    }

    std::size_t conn_count(std::memory_order order = std::memory_order_acquire) const noexcept {
        return cc_.load(order);
    }
};

} // namespace circ
} // namespace ipc
