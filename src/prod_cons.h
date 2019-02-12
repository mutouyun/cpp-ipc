#pragma once

#include <atomic>
#include <thread>
#include <utility>
#include <cstring>

#include "def.h"

#include "circ/elem_def.h"

namespace ipc {

////////////////////////////////////////////////////////////////
/// producer-consumer implementation
////////////////////////////////////////////////////////////////

template <typename Flag>
struct prod_cons_impl;

template <>
struct prod_cons_impl<wr<relat::single, relat::single, trans::unicast>> {
    std::atomic<circ::u2_t> rd_; // read index
    std::atomic<circ::u2_t> wt_; // write index

#if __cplusplus >= 201703L
    template <std::size_t DataSize>
    constexpr static std::size_t elem_param = DataSize - sizeof(circ::elem_head);
#else /*__cplusplus < 201703L*/
    template <std::size_t DataSize>
    struct elem_param {
        enum : std::size_t {
            value = DataSize - sizeof(circ::elem_head)
        };
    };
#endif/*__cplusplus < 201703L*/

    constexpr circ::u2_t cursor() const noexcept {
        return 0;
    }

    template <typename E, typename F, typename EB>
    bool push(E* /*elems*/, F&& f, EB* elem_start) {
        auto cur_wt = circ::index_of(wt_.load(std::memory_order_relaxed));
        if (cur_wt == circ::index_of(rd_.load(std::memory_order_acquire) - 1)) {
            return false; // full
        }
        std::forward<F>(f)(elem_start + cur_wt);
        wt_.fetch_add(1, std::memory_order_release);
        return true;
    }

    template <typename E, typename F, typename EB>
    bool pop(E* /*elems*/, circ::u2_t& /*cur*/, F&& f, EB* elem_start) {
        auto cur_rd = circ::index_of(rd_.load(std::memory_order_relaxed));
        if (cur_rd == circ::index_of(wt_.load(std::memory_order_acquire))) {
            return false; // empty
        }
        std::forward<F>(f)(elem_start + cur_rd);
        rd_.fetch_add(1, std::memory_order_release);
        return true;
    }
};

template <>
struct prod_cons_impl<wr<relat::single, relat::multi , trans::unicast>>
     : prod_cons_impl<wr<relat::single, relat::single, trans::unicast>> {

    template <typename E, typename F, typename EB>
    bool pop(E* /*elems*/, circ::u2_t& /*cur*/, F&& f, EB* elem_start) {
        byte_t buff[sizeof(E)];
        for (unsigned k = 0;;) {
            auto cur_rd = rd_.load(std::memory_order_relaxed);
            if (circ::index_of(cur_rd) ==
                circ::index_of(wt_.load(std::memory_order_acquire))) {
                return false; // empty
            }
            std::memcpy(buff, elem_start + circ::index_of(cur_rd), sizeof(buff));
            if (rd_.compare_exchange_weak(cur_rd, cur_rd + 1, std::memory_order_release)) {
                std::forward<F>(f)(buff);
                return true;
            }
            ipc::yield(k);
        }
    }
};

template <>
struct prod_cons_impl<wr<relat::multi , relat::multi, trans::unicast>>
     : prod_cons_impl<wr<relat::single, relat::multi, trans::unicast>> {

    std::atomic<circ::u2_t> ct_; // commit index

    template <typename E, typename F, typename EB>
    bool push(E* /*elems*/, F&& f, EB* elem_start) {
        circ::u2_t cur_ct, nxt_ct;
        while (1) {
            cur_ct = ct_.load(std::memory_order_relaxed);
            if (circ::index_of(nxt_ct = cur_ct + 1) ==
                circ::index_of(rd_.load(std::memory_order_acquire))) {
                return false; // full
            }
            if (ct_.compare_exchange_weak(cur_ct, nxt_ct, std::memory_order_release)) {
                break;
            }
            std::this_thread::yield();
        }
        std::forward<F>(f)(elem_start + circ::index_of(cur_ct));
        while (1) {
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
struct prod_cons_impl<wr<relat::single, relat::multi, trans::broadcast>> {
    std::atomic<circ::u2_t> wt_; // write index

#if __cplusplus >= 201703L
    template <std::size_t DataSize>
    constexpr static std::size_t elem_param = DataSize;
#else /*__cplusplus < 201703L*/
    template <std::size_t DataSize>
    struct elem_param { enum : std::size_t { value = DataSize }; };
#endif/*__cplusplus < 201703L*/

    /*
        <Remarks> std::atomic<T> may not have value_type.
        See: https://stackoverflow.com/questions/53648614/what-happened-to-stdatomicxvalue-type
    */
    using rc_t = decltype(circ::elem_head::rc_.load());

    circ::u2_t cursor() const noexcept {
        return wt_.load(std::memory_order_acquire);
    }

    template <typename E, typename F, typename EB>
    bool push(E* elems, F&& f, EB* elem_start) {
        auto conn_cnt = elems->conn_count(std::memory_order_relaxed);
        if (conn_cnt == 0) return false;
        auto el = elem_start + circ::index_of(wt_.load(std::memory_order_acquire));
        // check all consumers have finished reading this element
        while (1) {
            rc_t expected = 0;
            if (el->head_.rc_.compare_exchange_weak(
                        expected, static_cast<rc_t>(conn_cnt), std::memory_order_release)) {
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

    template <typename E, typename F, typename EB>
    bool pop(E* /*elems*/, circ::u2_t& cur, F&& f, EB* elem_start) {
        if (cur == cursor()) return false; // acquire
        auto el = elem_start + circ::index_of(cur++);
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
struct prod_cons_impl<wr<relat::multi , relat::multi, trans::broadcast>>
     : prod_cons_impl<wr<relat::single, relat::multi, trans::broadcast>> {

    std::atomic<circ::u2_t> ct_; // commit index

    template <typename E, typename F, typename EB>
    bool push(E* elems, F&& f, EB* elem_start) {
        auto conn_cnt = elems->conn_count(std::memory_order_relaxed);
        if (conn_cnt == 0) return false;
        circ::u2_t cur_ct = ct_.fetch_add(1, std::memory_order_acquire),
                   nxt_ct = cur_ct + 1;
        auto el = elem_start + circ::index_of(cur_ct);
        // check all consumers have finished reading this element
        while (1) {
            rc_t expected = 0;
            if (el->head_.rc_.compare_exchange_weak(
                        expected, static_cast<rc_t>(conn_cnt), std::memory_order_release)) {
                break;
            }
            std::this_thread::yield();
            conn_cnt = elems->conn_count(); // acquire
            if (conn_cnt == 0) return false;
        }
        std::forward<F>(f)(el->data_);
        while (1) {
            auto exp_wt = cur_ct;
            if (wt_.compare_exchange_weak(exp_wt, nxt_ct, std::memory_order_release)) {
                break;
            }
            std::this_thread::yield();
        }
        return true;
    }
};

} // namespace ipc
