#pragma once

#include <atomic>
#include <utility>
#include <algorithm>
#include <thread>

#include "def.h"

namespace ipc {
namespace circ {

template <std::size_t N>
struct alignas(std::max_align_t) elem_array_head {
    using u1_t = uint_t<N>;
    using u2_t = uint_t<N * 2>;

    std::atomic<u2_t> cc_ { 0 }; // connection counter, using for broadcast
    std::atomic<u2_t> wt_ { 0 }; // write index

    constexpr static u1_t index_of(u2_t c) noexcept {
        return static_cast<u1_t>(c);
    }

    std::size_t connect() noexcept {
        return cc_.fetch_add(1, std::memory_order_release);
    }

    std::size_t disconnect() noexcept {
        return cc_.fetch_sub(1, std::memory_order_release);
    }

    std::size_t conn_count() const noexcept {
        return cc_.load(std::memory_order_acquire);
    }

    u2_t cursor() const noexcept {
        return wt_.load(std::memory_order_acquire);
    }

    auto acquire(std::memory_order order = std::memory_order_acquire) noexcept {
        return index_of(wt_.load(order));
    }

    void commit() noexcept {
        wt_.fetch_add(1, std::memory_order_release);
    }
};

template <std::size_t N>
constexpr std::size_t elem_array_head_size =
    (sizeof(elem_array_head<N>) % alignof(std::max_align_t)) ?
   ((sizeof(elem_array_head<N>) / alignof(std::max_align_t)) + 1) * alignof(std::max_align_t) :
     sizeof(elem_array_head<N>);

struct elem_head {
    std::atomic<uint_t<32>> rc_ { 0 }; // read counter
};

template <std::size_t DataSize, std::size_t BaseIntSize = 8>
class elem_array : protected elem_array_head<BaseIntSize> {
public:
    using base_t = elem_array_head<BaseIntSize>;
    using head_t = elem_head;

    using typename base_t::u1_t;
    using typename base_t::u2_t;

    enum : std::size_t {
        head_size  = elem_array_head_size<BaseIntSize>,
        data_size  = DataSize,
        elem_max   = (std::numeric_limits<u1_t>::max)() + 1, // default is 255 + 1
        elem_size  = sizeof(head_t) + DataSize,
        block_size = elem_size * elem_max
    };

protected:
    struct elem_t {
        head_t head_;
        byte_t data_[data_size] {};
    };
    elem_t block_[elem_max];

    elem_t* elem_start() noexcept {
        return block_;
    }

    static elem_t* elem(void* ptr) noexcept { return reinterpret_cast<elem_t*>(static_cast<byte_t*>(ptr) - sizeof(head_t)); }
           elem_t* elem(u1_t i   ) noexcept { return elem_start() + i; }

    template <typename Acq, typename... P>
    void* acquire(std::memory_order order, Acq&& acq, P&&... params) noexcept {
        uint_t<32> conn_cnt = cc_.load(order);
        if (conn_cnt == 0) return nullptr;
        elem_t* el = elem(std::forward<Acq>(acq)(std::memory_order_relaxed,
                                                 std::forward<P>(params)...));
        // check all consumers have finished reading
        while(1) {
            uint_t<32> expected = 0;
            if (el->head_.rc_.compare_exchange_weak(
                        expected, conn_cnt, std::memory_order_relaxed)) {
                break;
            }
            std::this_thread::yield();
            conn_cnt = cc_.load(std::memory_order_acquire);
        }
        return el->data_;
    }

public:
    elem_array() = default;

    elem_array(const elem_array&) = delete;
    elem_array& operator=(const elem_array&) = delete;
    elem_array(elem_array&&) = delete;
    elem_array& operator=(elem_array&&) = delete;

    using base_t::connect;
    using base_t::disconnect;
    using base_t::conn_count;
    using base_t::cursor;

    void* acquire(std::memory_order order = std::memory_order_acquire) noexcept {
        return this->acquire(order, [this](auto o) {
            return base_t::acquire(o);
        });
    }

    void commit(void* /*ptr*/) noexcept {
        base_t::commit();
    }

    template <typename F>
    bool fetch(F&& f) noexcept {
        auto p = this->acquire();
        if (p == nullptr) return false;
        std::forward<F>(f)(p);
        this->commit(p);
        return true;
    }

    void* take(u2_t cursor) noexcept {
        return elem(base_t::index_of(cursor))->data_;
    }

    void put(void* ptr) noexcept {
        auto el = elem(ptr);
        uint_t<32> cur_rc;
        do {
            cur_rc = el->head_.rc_.load(std::memory_order_relaxed);
            if (cur_rc == 0) return;
        } while (!el->head_.rc_.compare_exchange_weak(
                     cur_rc, cur_rc - 1, std::memory_order_release));
    }
};
/*
template <std::size_t DataSize, std::size_t BaseIntSize = 8>
class multi_write_array : protected elem_array<DataSize, BaseIntSize> {
public:
    using base_t = elem_array<DataSize, BaseIntSize>;
    using head_t = typename base_t::head_t;

    using typename base_t::u1_t;
    using typename base_t::u2_t;

    using base_t::head_size;
    using base_t::data_size;
    using base_t::elem_max;
    using base_t::elem_size;
    using base_t::block_size;

protected:
    std::atomic<u2_t> rd_ { 0 }; // ready index

public:
    using base_t::connect;
    using base_t::disconnect;
    using base_t::conn_count;

    u2_t cursor() const noexcept {
        return rd_.load(std::memory_order_acquire);
    }

    template <typename F>
    bool fetch(F&& f) noexcept {
        u2_t cur_rd;
        auto p = base_t::acquire(std::memory_order_acquire, [this, &cur_rd](auto o) {
            while (1) {
                u2_t cur_wt = wt_.load(o), nxt_wt = cur_wt + 1;
                if (base_t::index_of(nxt_wt) ==
                    base_t::index_of(cur_rd = rd_.load(std::memory_order_relaxed))) {
                    // is full
                }
                else if (wt_.compare_exchange_weak(cur_wt, nxt_wt, std::memory_order_relaxed)) {
                    return base_t::index_of(nxt_wt);
                }
                std::this_thread::yield();
                std::atomic_thread_fence(std::memory_order_acquire);
            }
        });
        if (p == nullptr) return false;
        std::forward<F>(f)(p);
        while (1) {
            if (rd_.compare_exchange_weak(cur_rd, cur_rd + 1, std::memory_order_release)) {
                break;
            }
            std::this_thread::yield();
        }
        return true;
    }
};
*/
} // namespace circ
} // namespace ipc
