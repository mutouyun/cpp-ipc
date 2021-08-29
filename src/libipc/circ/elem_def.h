#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <new>

#include "libipc/def.h"
#include "libipc/rw_lock.h"

#include "libipc/platform/detail.h"

namespace ipc {
namespace circ {

using u1_t = ipc::uint_t<8>;
using u2_t = ipc::uint_t<32>;

/** only supports max 32 connections in broadcast mode */
using cc_t = u2_t;

constexpr u1_t index_of(u2_t c) noexcept {
    return static_cast<u1_t>(c);
}

class conn_head_base {
protected:
    std::atomic<cc_t> cc_{0}; // connections
    ipc::spin_lock lc_;
    std::atomic<bool> constructed_{false};

public:
    void init() {
        /* DCLP */
        if (!constructed_.load(std::memory_order_acquire)) {
            IPC_UNUSED_ auto guard = ipc::detail::unique_lock(lc_);
            if (!constructed_.load(std::memory_order_relaxed)) {
                ::new (this) conn_head_base;
                constructed_.store(true, std::memory_order_release);
            }
        }
    }

    conn_head_base() = default;
    conn_head_base(conn_head_base const &) = delete;
    conn_head_base &operator=(conn_head_base const &) = delete;

    cc_t connections(std::memory_order order = std::memory_order_acquire) const noexcept {
        return this->cc_.load(order);
    }
};

template <typename P, bool = relat_trait<P>::is_broadcast>
class conn_head;

template <typename P>
class conn_head<P, true> : public conn_head_base {
public:
    cc_t connect() noexcept {
        for (unsigned k = 0;; ipc::yield(k)) {
            cc_t curr = this->cc_.load(std::memory_order_acquire);
            cc_t next = curr | (curr + 1); // find the first 0, and set it to 1.
            if (next == 0) {
                // connection-slot is full.
                return 0;
            }
            if (this->cc_.compare_exchange_weak(curr, next, std::memory_order_release)) {
                return next ^ curr; // return connected id
            }
        }
    }

    cc_t disconnect(cc_t cc_id) noexcept {
        return this->cc_.fetch_and(~cc_id, std::memory_order_acq_rel) & ~cc_id;
    }

    std::size_t conn_count(std::memory_order order = std::memory_order_acquire) const noexcept {
        cc_t cur = this->cc_.load(order);
        cc_t cnt; // accumulates the total bits set in cc
        for (cnt = 0; cur; ++cnt) cur &= cur - 1;
        return cnt;
    }
};

template <typename P>
class conn_head<P, false> : public conn_head_base {
public:
    cc_t connect() noexcept {
        return this->cc_.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    cc_t disconnect(cc_t cc_id) noexcept {
        if (cc_id == ~static_cast<circ::cc_t>(0u)) {
            // clear all connections
            this->cc_.store(0, std::memory_order_relaxed);
            return 0u;
        }
        else {
            return this->cc_.fetch_sub(1, std::memory_order_relaxed) - 1;
        }
    }

    std::size_t conn_count(std::memory_order order = std::memory_order_acquire) const noexcept {
        return this->connections(order);
    }
};

} // namespace circ
} // namespace ipc
