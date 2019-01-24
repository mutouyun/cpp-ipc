#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <new>

#include "rw_lock.h"

#include "platform/waiter.h"
#include "platform/detail.h"

namespace ipc {
namespace circ {

struct elem_head {
    std::atomic<std::size_t> rc_ { 0 }; // read-counter
};

template <std::size_t DataSize>
struct elem_t {
    elem_head head_;
    byte_t    data_[DataSize] {};
};

template <>
struct elem_t<0> {
    elem_head head_;
};

template <std::size_t S>
elem_t<S>* elem_of(void* ptr) noexcept {
    return reinterpret_cast<elem_t<S>*>(static_cast<byte_t*>(ptr) - sizeof(elem_head));
}

using u1_t = ipc::uint_t<8>;
using u2_t = ipc::uint_t<16>;

constexpr u1_t index_of(u2_t c) noexcept {
    return static_cast<u1_t>(c);
}

class conn_head {
    ipc::detail::waiter      cc_waiter_, waiter_;
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
                ::printf("init...\n");
            }
        }
    }

    conn_head() = default;
    conn_head(const conn_head&) = delete;
    conn_head& operator=(const conn_head&) = delete;

    auto       & waiter()       noexcept { return this->waiter_; }
    auto const & waiter() const noexcept { return this->waiter_; }

    auto       & conn_waiter()       noexcept { return this->cc_waiter_; }
    auto const & conn_waiter() const noexcept { return this->cc_waiter_; }

    std::size_t connect() noexcept {
        return cc_.fetch_add(1, std::memory_order_release);
    }

    std::size_t disconnect() noexcept {
        return cc_.fetch_sub(1, std::memory_order_release);
    }

    std::size_t conn_count(std::memory_order order = std::memory_order_acquire) const noexcept {
        return cc_.load(order);
    }
};

} // namespace circ
} // namespace ipc
