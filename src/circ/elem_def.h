#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <new>

#include "rw_lock.h"

#include "platform/detail.h"

namespace ipc {
namespace circ {

using u1_t = ipc::uint_t<8>;
using u2_t = ipc::uint_t<32>;

/** only supports max 32 connections */
using cc_t = u2_t;

constexpr u1_t index_of(u2_t c) noexcept {
    return static_cast<u1_t>(c);
}

class conn_head {
    std::atomic<cc_t> cc_; // connections
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

    cc_t connect() noexcept {
        for (unsigned k = 0;;) {
            cc_t cur = cc_.load(std::memory_order_acquire);
            cc_t next = cur | (cur + 1); // find the first 0, and set it to 1.
            if (next == 0) return 0;
            if (cc_.compare_exchange_weak(cur, next, std::memory_order_release)) {
                return next ^ cur; // return connected id
            }
            ipc::yield(k);
        }
    }

    cc_t disconnect(cc_t cc_id) noexcept {
        return cc_.fetch_and(~cc_id) & ~cc_id;
    }

    cc_t connections(std::memory_order order = std::memory_order_acquire) const noexcept {
        return cc_.load(order);
    }

    std::size_t conn_count(std::memory_order order = std::memory_order_acquire) const noexcept {
        cc_t cur = cc_.load(order);
        cc_t cnt; // accumulates the total bits set in cc
        for (cnt = 0; cur; ++cnt) cur &= cur - 1;
        return cnt;
    }
};

} // namespace circ
} // namespace ipc
