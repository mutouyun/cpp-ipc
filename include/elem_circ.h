#pragma once

#include <atomic>
#include <thread>
#include <cstring>
#include <utility>

#include "def.h"
#include "rw_lock.h"
#include "elem_def.h"

#include "platform/waiter.h"

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
} // namespace circ

////////////////////////////////////////////////////////////////
/// producer-consumer policies
////////////////////////////////////////////////////////////////

template <>
struct prod_cons<orgnz::cyclic, relat::single, relat::single, trans::unicast> {
    std::atomic<circ::detail::u2_t> rd_ { 0 }; // read index
    std::atomic<circ::detail::u2_t> wt_ { 0 }; // write index

    template <std::size_t DataSize>
    constexpr static std::size_t elem_param = DataSize - sizeof(circ::detail::elem_head);

    constexpr circ::detail::u2_t cursor() const noexcept {
        return 0;
    }

    template <typename E, typename F, std::size_t S>
    bool push(E* /*elems*/, F&& f, circ::detail::elem_t<S>* elem_start) {
        auto cur_wt = circ::detail::index_of(wt_.load(std::memory_order_acquire));
        if (cur_wt == circ::detail::index_of(rd_.load(std::memory_order_relaxed) - 1)) {
            return false; // full
        }
        std::forward<F>(f)(elem_start + cur_wt);
        wt_.fetch_add(1, std::memory_order_release);
        return true;
    }

    template <typename E, typename F, std::size_t S>
    bool pop(E* /*elems*/, circ::detail::u2_t& /*cur*/, F&& f, circ::detail::elem_t<S>* elem_start) noexcept {
        auto cur_rd = circ::detail::index_of(rd_.load(std::memory_order_acquire));
        if (cur_rd == circ::detail::index_of(wt_.load(std::memory_order_relaxed))) {
            return false; // empty
        }
        std::forward<F>(f)(elem_start + cur_rd);
        rd_.fetch_add(1, std::memory_order_release);
        return true;
    }
};

template <>
struct prod_cons<orgnz::cyclic, relat::single, relat::multi , trans::unicast>
     : prod_cons<orgnz::cyclic, relat::single, relat::single, trans::unicast> {

    template <typename E, typename F, std::size_t S>
    bool pop(E* /*elems*/, circ::detail::u2_t& /*cur*/, F&& f, circ::detail::elem_t<S>* elem_start) noexcept {
        byte_t buff[sizeof(circ::detail::elem_t<S>)];
        for (unsigned k = 0;;) {
            auto cur_rd = rd_.load(std::memory_order_acquire);
            if (circ::detail::index_of(cur_rd) ==
                circ::detail::index_of(wt_.load(std::memory_order_relaxed))) {
                return false; // empty
            }
            std::memcpy(buff, elem_start + circ::detail::index_of(cur_rd), sizeof(buff));
            if (rd_.compare_exchange_weak(cur_rd, cur_rd + 1, std::memory_order_release)) {
                std::forward<F>(f)(buff);
                return true;
            }
            ipc::yield(k);
        }
    }
};

template <>
struct prod_cons<orgnz::cyclic, relat::multi , relat::multi, trans::unicast>
     : prod_cons<orgnz::cyclic, relat::single, relat::multi, trans::unicast> {

    std::atomic<circ::detail::u2_t> ct_ { 0 }; // commit index

    template <typename E, typename F, std::size_t S>
    bool push(E* /*elems*/, F&& f, circ::detail::elem_t<S>* elem_start) {
        circ::detail::u2_t cur_ct, nxt_ct;
        while(1) {
            cur_ct = ct_.load(std::memory_order_acquire);
            if (circ::detail::index_of(nxt_ct = cur_ct + 1) ==
                circ::detail::index_of(rd_.load(std::memory_order_relaxed))) {
                return false; // full
            }
            if (ct_.compare_exchange_weak(cur_ct, nxt_ct, std::memory_order_relaxed)) {
                break;
            }
            std::this_thread::yield();
        }
        std::forward<F>(f)(elem_start + circ::detail::index_of(cur_ct));
        while(1) {
            auto exp_wt = cur_ct;
            if (wt_.compare_exchange_weak(exp_wt, nxt_ct, std::memory_order_release)) {
                break;
            }
            std::this_thread::yield();
        }
        return true;
    }
};

template <>
struct prod_cons<orgnz::cyclic, relat::single, relat::multi, trans::broadcast> {
    std::atomic<circ::detail::u2_t> wt_ { 0 }; // write index

    template <std::size_t DataSize>
    constexpr static std::size_t elem_param = DataSize;

    /*
        <Remarks> std::atomic<T> may not have value_type.
        See: https://stackoverflow.com/questions/53648614/what-happened-to-stdatomicxvalue-type
    */
    using rc_t = decltype(circ::detail::elem_head::rc_.load());

    circ::detail::u2_t cursor() const noexcept {
        return wt_.load(std::memory_order_acquire);
    }

    template <typename E, typename F, std::size_t S>
    bool push(E* elems, F&& f, circ::detail::elem_t<S>* elem_start) {
        auto conn_cnt = elems->conn_count(); // acquire
        if (conn_cnt == 0) return false;
        auto el = elem_start + circ::detail::index_of(wt_.load(std::memory_order_relaxed));
        // check all consumers have finished reading this element
        while(1) {
            rc_t expected = 0;
            if (el->head_.rc_.compare_exchange_weak(
                        expected, static_cast<rc_t>(conn_cnt), std::memory_order_relaxed)) {
                break;
            }
            std::this_thread::yield();
            conn_cnt = elems->conn_count(); // acquire
            if (conn_cnt == 0) return false;
        }
        std::forward<F>(f)(el->data_);
        wt_.fetch_add(1, std::memory_order_release);
        return true;
    }

    template <typename E, typename F, std::size_t S>
    bool pop(E* /*elems*/, circ::detail::u2_t& cur, F&& f, circ::detail::elem_t<S>* elem_start) noexcept {
        if (cur == cursor()) return false; // acquire
        auto el = elem_start + circ::detail::index_of(cur++);
        std::forward<F>(f)(el->data_);
        for (unsigned k = 0;;) {
            rc_t cur_rc = el->head_.rc_.load(std::memory_order_acquire);
            if (cur_rc == 0) {
                return true;
            }
            if (el->head_.rc_.compare_exchange_weak(
                        cur_rc, cur_rc - 1, std::memory_order_release)) {
                return true;
            }
            ipc::yield(k);
        }
    }
};

template <>
struct prod_cons<orgnz::cyclic, relat::multi , relat::multi, trans::broadcast>
     : prod_cons<orgnz::cyclic, relat::single, relat::multi, trans::broadcast> {

    std::atomic<circ::detail::u2_t> ct_ { 0 }; // commit index

    template <typename E, typename F, std::size_t S>
    bool push(E* elems, F&& f, circ::detail::elem_t<S>* elem_start) {
        auto conn_cnt = elems->conn_count(); // acquire
        if (conn_cnt == 0) return false;
        circ::detail::u2_t cur_ct = ct_.fetch_add(1, std::memory_order_relaxed),
                           nxt_ct = cur_ct + 1;
        auto el = elem_start + circ::detail::index_of(cur_ct);
        // check all consumers have finished reading this element
        while(1) {
            rc_t expected = 0;
            if (el->head_.rc_.compare_exchange_weak(
                        expected, static_cast<rc_t>(conn_cnt), std::memory_order_relaxed)) {
                break;
            }
            std::this_thread::yield();
            conn_cnt = elems->conn_count(); // acquire
            if (conn_cnt == 0) return false;
        }
        std::forward<F>(f)(el->data_);
        while(1) {
            auto exp_wt = cur_ct;
            if (wt_.compare_exchange_weak(exp_wt, nxt_ct, std::memory_order_release)) {
                break;
            }
            std::this_thread::yield();
        }
        return true;
    }
};

template <relat Rp, relat Rc, trans Ts>
using prod_cons_circ = prod_cons<orgnz::cyclic, Rp, Rc, Ts>;

namespace circ {

////////////////////////////////////////////////////////////////
/// element-array implementation
////////////////////////////////////////////////////////////////

template <std::size_t DataSize, typename Policy>
class elem_array : private Policy {
public:
    using policy_t = Policy;
    using base_t   = Policy;
    using head_t   = ipc::conn_head<detail::u2_t>;
    using elem_t   = detail::elem_t<policy_t::template elem_param<DataSize>>;

    enum : std::size_t {
        head_size  = sizeof(policy_t) + sizeof(head_t),
        data_size  = DataSize,
        elem_max   = (std::numeric_limits<uint_t<8>>::max)() + 1, // default is 255 + 1
        elem_size  = sizeof(elem_t),
        block_size = elem_size * elem_max
    };

private:
    head_t head_;
    ipc::detail::waiter waiter_;
    elem_t block_[elem_max];

public:
    elem_array() = default;
    elem_array(const elem_array&) = delete;
    elem_array& operator=(const elem_array&) = delete;

    auto       & waiter()       { return this->waiter_; }
    auto const & waiter() const { return this->waiter_; }

    auto       & conn_waiter()       { return head_.conn_waiter(); }
    auto const & conn_waiter() const { return head_.conn_waiter(); }

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
