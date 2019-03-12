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

    template <std::size_t DataSize>
    struct elem_t {
        byte_t data_[DataSize] {};
    };

    alignas(circ::cache_line_size) std::atomic<circ::u2_t> rd_; // read index
    alignas(circ::cache_line_size) std::atomic<circ::u2_t> wt_; // write index

    constexpr circ::u2_t cursor() const noexcept {
        return 0;
    }

    template <typename W, typename F, template <std::size_t> class E, std::size_t DataSize>
    bool push(W* /*wrapper*/, F&& f, E<DataSize>* elems) {
        auto cur_wt = circ::index_of(wt_.load(std::memory_order_relaxed));
        if (cur_wt == circ::index_of(rd_.load(std::memory_order_acquire) - 1)) {
            return false; // full
        }
        std::forward<F>(f)(&(elems[cur_wt].data_));
        wt_.fetch_add(1, std::memory_order_release);
        return true;
    }

    template <typename W, typename F, template <std::size_t> class E, std::size_t DataSize>
    bool pop(W* /*wrapper*/, circ::u2_t& /*cur*/, F&& f, E<DataSize>* elems) {
        auto cur_rd = circ::index_of(rd_.load(std::memory_order_relaxed));
        if (cur_rd == circ::index_of(wt_.load(std::memory_order_acquire))) {
            return false; // empty
        }
        std::forward<F>(f)(&(elems[cur_rd].data_));
        rd_.fetch_add(1, std::memory_order_release);
        return true;
    }
};

template <>
struct prod_cons_impl<wr<relat::single, relat::multi , trans::unicast>>
     : prod_cons_impl<wr<relat::single, relat::single, trans::unicast>> {

    template <typename W, typename F, template <std::size_t> class E, std::size_t DataSize>
    bool pop(W* /*wrapper*/, circ::u2_t& /*cur*/, F&& f, E<DataSize>* elems) {
        byte_t buff[DataSize];
        for (unsigned k = 0;;) {
            auto cur_rd = rd_.load(std::memory_order_relaxed);
            if (circ::index_of(cur_rd) ==
                circ::index_of(wt_.load(std::memory_order_acquire))) {
                return false; // empty
            }
            std::memcpy(buff, &(elems[circ::index_of(cur_rd)].data_), sizeof(buff));
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

    enum : std::uint64_t {
        invalid_index = (std::numeric_limits<std::uint64_t>::max)()
    };

    template <std::size_t DataSize>
    struct elem_t {
        byte_t data_[DataSize] {};
        alignas(circ::cache_line_size) std::atomic<std::uint64_t> f_ct_ { invalid_index }; // commit flag
    };

    alignas(circ::cache_line_size) std::atomic<circ::u2_t> ct_; // commit index
    alignas(circ::cache_line_size) std::atomic<unsigned  > barrier_;

    template <typename W, typename F, template <std::size_t> class E, std::size_t DataSize>
    bool push(W* /*wrapper*/, F&& f, E<DataSize>* elems) {
        circ::u2_t cur_ct, nxt_ct;
        for (unsigned k = 0;;) {
            cur_ct = ct_.load(std::memory_order_relaxed);
            if (circ::index_of(nxt_ct = cur_ct + 1) ==
                circ::index_of(rd_.load(std::memory_order_acquire))) {
                return false; // full
            }
            if (ct_.compare_exchange_weak(cur_ct, nxt_ct, std::memory_order_release)) {
                break;
            }
            ipc::yield(k);
        }
        auto* el = elems + circ::index_of(cur_ct);
        std::forward<F>(f)(&(el->data_));
        // set flag & try update wt
        el->f_ct_.store(cur_ct, std::memory_order_release);
        while (1) {
            barrier_.exchange(0, std::memory_order_acq_rel);
            auto cac_ct = el->f_ct_.load(std::memory_order_acquire);
            if (cur_ct != wt_.load(std::memory_order_acquire)) {
                return true;
            }
            if (cac_ct != cur_ct) {
                return true;
            }
            if (!el->f_ct_.compare_exchange_strong(cac_ct, invalid_index, std::memory_order_relaxed)) {
                return true;
            }
            wt_.store(nxt_ct, std::memory_order_release);
            cur_ct = nxt_ct;
            nxt_ct = cur_ct + 1;
            el = elems + circ::index_of(cur_ct);
        }
        return true;
    }
};

template <>
struct prod_cons_impl<wr<relat::single, relat::multi, trans::broadcast>> {

    using rc_t = std::size_t;

    template <std::size_t DataSize>
    struct elem_t {
        byte_t data_[DataSize] {};
        alignas(circ::cache_line_size) std::atomic<rc_t> rc_ { 0 }; // read-counter
    };

    alignas(circ::cache_line_size) std::atomic<circ::u2_t> wt_; // write index

    circ::u2_t cursor() const noexcept {
        return wt_.load(std::memory_order_acquire);
    }

    template <typename W, typename F, template <std::size_t> class E, std::size_t DataSize>
    bool push(W* wrapper, F&& f, E<DataSize>* elems) {
        auto conn_cnt = wrapper->conn_count(std::memory_order_relaxed);
        if (conn_cnt == 0) return false;
        auto el = elems + circ::index_of(wt_.load(std::memory_order_acquire));
        // check all consumers have finished reading this element
        while (1) {
            rc_t expected = 0;
            if (el->rc_.compare_exchange_weak(
                        expected, static_cast<rc_t>(conn_cnt), std::memory_order_release)) {
                break;
            }
            std::this_thread::yield();
            conn_cnt = wrapper->conn_count(); // acquire
            if (conn_cnt == 0) return false;
        }
        std::forward<F>(f)(&(el->data_));
        wt_.fetch_add(1, std::memory_order_release);
        return true;
    }

    template <typename W, typename F, template <std::size_t> class E, std::size_t DataSize>
    bool pop(W* /*wrapper*/, circ::u2_t& cur, F&& f, E<DataSize>* elems) {
        if (cur == cursor()) return false; // acquire
        auto el = elems + circ::index_of(cur++);
        std::forward<F>(f)(&(el->data_));
        for (unsigned k = 0;;) {
            rc_t cur_rc = el->rc_.load(std::memory_order_acquire);
            if (cur_rc == 0) {
                return true;
            }
            if (el->rc_.compare_exchange_weak(
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

    alignas(circ::cache_line_size) std::atomic<circ::u2_t> ct_; // commit index

    template <typename W, typename F, template <std::size_t> class E, std::size_t DataSize>
    bool push(W* wrapper, F&& f, E<DataSize>* elems) {
        auto conn_cnt = wrapper->conn_count(std::memory_order_relaxed);
        if (conn_cnt == 0) return false;
        circ::u2_t cur_ct = ct_.fetch_add(1, std::memory_order_acquire),
                   nxt_ct = cur_ct + 1;
        auto el = elems + circ::index_of(cur_ct);
        // check all consumers have finished reading this element
        while (1) {
            rc_t expected = 0;
            if (el->rc_.compare_exchange_weak(
                        expected, static_cast<rc_t>(conn_cnt), std::memory_order_release)) {
                break;
            }
            std::this_thread::yield();
            conn_cnt = wrapper->conn_count(); // acquire
            if (conn_cnt == 0) return false;
        }
        std::forward<F>(f)(&(el->data_));
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
