#pragma once

#include <atomic>
#include <thread>
#include <cstring>

#include "def.h"

namespace ipc {
namespace circ {

namespace detail {

using u1_t = uint_t<8>;
using u2_t = uint_t<16>;

constexpr u1_t index_of(u2_t c) noexcept {
    return static_cast<u1_t>(c);
}

struct elem_head {
    std::atomic<std::size_t> rc_ { 0 }; // read counter
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

} // namespace detail

enum class relat { // multiplicity of the relationship
    single,
    multi
};

enum class trans { // transmission
    unicast,
    broadcast
};

////////////////////////////////////////////////////////////////
/// producer-consumer policies
////////////////////////////////////////////////////////////////

template <relat Rp, relat Rc, trans Ts>
struct prod_cons;

template <>
struct prod_cons<relat::single, relat::single, trans::unicast> {
    std::atomic<detail::u2_t> rd_ { 0 }; // read index
    std::atomic<detail::u2_t> wt_ { 0 }; // write index

    template <std::size_t DataSize>
    constexpr static std::size_t elem_param = DataSize - sizeof(detail::elem_head);

    constexpr detail::u2_t cursor() const noexcept {
        return 0;
    }

    template <typename E, typename F, std::size_t S>
    bool push(E* /*elems*/, F&& f, detail::elem_t<S>* elem_start) {
        auto cur_wt = detail::index_of(wt_.load(std::memory_order_acquire));
        if (cur_wt == detail::index_of(rd_.load(std::memory_order_relaxed) - 1)) {
            return false;
        }
        std::forward<F>(f)(elem_start + cur_wt);
        wt_.fetch_add(1, std::memory_order_release);
        return true;
    }

    template <typename E, typename F, std::size_t S>
    bool pop(E* /*elems*/, detail::u2_t& /*cur*/, F&& f, detail::elem_t<S>* elem_start) noexcept {
        auto cur_rd = detail::index_of(rd_.load(std::memory_order_acquire));
        if (cur_rd == detail::index_of(wt_.load(std::memory_order_relaxed))) {
            return false;
        }
        std::forward<F>(f)(elem_start + cur_rd);
        rd_.fetch_add(1, std::memory_order_release);
        return true;
    }
};

template <>
struct prod_cons<relat::single, relat::multi, trans::unicast>
     : prod_cons<relat::single, relat::single, trans::unicast> {

    template <typename E, typename F, std::size_t S>
    bool pop(E* /*elems*/, detail::u2_t& /*cur*/, F&& f, detail::elem_t<S>* elem_start) noexcept {
        byte_t buff[sizeof(detail::elem_t<S>)];
        while (1) {
            auto cur_rd = rd_.load(std::memory_order_acquire);
            if (detail::index_of(cur_rd) ==
                detail::index_of(wt_.load(std::memory_order_relaxed))) {
                return false;
            }
            std::memcpy(buff, elem_start + detail::index_of(cur_rd), sizeof(buff));
            if (rd_.compare_exchange_weak(cur_rd, cur_rd + 1, std::memory_order_release)) {
                std::forward<F>(f)(buff);
                return true;
            }
            std::this_thread::yield();
        }
    }
};

template <>
struct prod_cons<relat::single, relat::multi, trans::broadcast> {
    std::atomic<detail::u2_t> wt_ { 0 }; // write index

    template <std::size_t DataSize>
    constexpr static std::size_t elem_param = DataSize;

    /*
        <Remarks> std::atomic<T> may not have value_type.
        See: https://stackoverflow.com/questions/53648614/what-happened-to-stdatomicxvalue-type
    */
    using rc_t = decltype(detail::elem_head::rc_.load());

    detail::u2_t cursor() const noexcept {
        return wt_.load(std::memory_order_acquire);
    }

    template <typename E, typename F, std::size_t S>
    bool push(E* elems, F&& f, detail::elem_t<S>* elem_start) {
        auto conn_cnt = elems->conn_count(); // acquire
        if (conn_cnt == 0) return false;
        auto el = elem_start + detail::index_of(wt_.load(std::memory_order_relaxed));
        // check all consumers have finished reading this element
        rc_t expected = 0;
        if (!el->head_.rc_.compare_exchange_weak(
                    expected, static_cast<rc_t>(conn_cnt), std::memory_order_relaxed)) {
            return false;
        }
        std::forward<F>(f)(el->data_);
        wt_.fetch_add(1, std::memory_order_release);
        return true;
    }

    template <typename E, typename F, std::size_t S>
    bool pop(E* /*elems*/, detail::u2_t& cur, F&& f, detail::elem_t<S>* elem_start) noexcept {
        if (cur == cursor()) return false; // acquire
        auto el = elem_start + detail::index_of(cur++);
        std::forward<F>(f)(el->data_);
        do {
            rc_t cur_rc = el->head_.rc_.load(std::memory_order_acquire);
            if (cur_rc == 0) {
                return true;
            }
            if (el->head_.rc_.compare_exchange_weak(
                        cur_rc, cur_rc - 1, std::memory_order_release)) {
                return true;
            }
            std::this_thread::yield();
        } while (1);
    }
};

////////////////////////////////////////////////////////////////
/// element-array implementation
////////////////////////////////////////////////////////////////

struct elems_head {
    std::atomic<detail::u2_t> cc_ { 0 }; // connection counter

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

template <std::size_t DataSize, typename Policy>
class elems_array : private Policy {
public:
    using policy_t = Policy;
    using base_t   = Policy;
    using head_t   = elems_head;
    using elem_t   = detail::elem_t<policy_t::template elem_param<DataSize>>;

    enum : std::size_t {
        head_size  = sizeof(head_t),
        data_size  = DataSize,
        elem_max   = (std::numeric_limits<uint_t<8>>::max)() + 1, // default is 255 + 1
        elem_size  = sizeof(elem_t),
        block_size = elem_size * elem_max
    };

private:
    head_t head_;
    elem_t block_[elem_max];

public:
    elems_array() = default;
    elems_array(const elems_array&) = delete;
    elems_array& operator=(const elems_array&) = delete;

    std::size_t connect   ()       noexcept { return head_.connect   (); }
    std::size_t disconnect()       noexcept { return head_.disconnect(); }
    std::size_t conn_count() const noexcept { return head_.conn_count(); }

    using base_t::cursor;

    template <typename F>
    bool push(F&& f) noexcept {
        return base_t::push(this, std::forward<F>(f), block_);
    }

    template <typename F>
    bool pop(detail::u2_t& cur, F&& f) noexcept {
        return base_t::pop(this, cur, std::forward<F>(f), block_);
    }
};

} // namespace circ
} // namespace ipc
