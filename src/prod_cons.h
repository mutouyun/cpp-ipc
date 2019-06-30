#pragma once

#include <atomic>
#include <utility>
#include <cstring>
#include <type_traits>

#include "def.h"
#include "platform/detail.h"
#include "circ/elem_def.h"

namespace ipc {

////////////////////////////////////////////////////////////////
/// producer-consumer implementation
////////////////////////////////////////////////////////////////

template <typename Flag>
struct prod_cons_impl;

template <>
struct prod_cons_impl<wr<relat::single, relat::single, trans::unicast>> {

    template <std::size_t DataSize, std::size_t AlignSize>
    struct elem_t {
        std::aligned_storage_t<DataSize, AlignSize> data_ {};
    };

    alignas(detail::cache_line_size) std::atomic<circ::u2_t> rd_; // read index
    alignas(detail::cache_line_size) std::atomic<circ::u2_t> wt_; // write index

    constexpr circ::u2_t cursor() const noexcept {
        return 0;
    }

    template <typename W, typename F, typename E>
    bool push(W* /*wrapper*/, F&& f, E* elems) {
        auto cur_wt = circ::index_of(wt_.load(std::memory_order_relaxed));
        if (cur_wt == circ::index_of(rd_.load(std::memory_order_acquire) - 1)) {
            return false; // full
        }
        std::forward<F>(f)(&(elems[cur_wt].data_));
        wt_.fetch_add(1, std::memory_order_release);
        return true;
    }

    template <typename W, typename F, typename E>
    bool force_push(W* wrapper, F&& f, E* elems) {
        return push(wrapper, std::forward<F>(f), elems);
    }

    template <typename W, typename F, typename E>
    bool pop(W* /*wrapper*/, circ::u2_t& /*cur*/, F&& f, E* elems) {
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

    template <typename W, typename F, template <std::size_t, std::size_t> class E, std::size_t DS, std::size_t AS>
    bool pop(W* /*wrapper*/, circ::u2_t& /*cur*/, F&& f, E<DS, AS>* elems) {
        byte_t buff[DS];
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

    using flag_t = std::uint64_t;

    template <std::size_t DataSize, std::size_t AlignSize>
    struct elem_t {
        std::aligned_storage_t<DataSize, AlignSize> data_ {};
        std::atomic<flag_t> f_ct_ { 0 }; // commit flag
    };

    alignas(detail::cache_line_size) std::atomic<circ::u2_t> ct_; // commit index

    template <typename W, typename F, typename E>
    bool push(W* /*wrapper*/, F&& f, E* elems) {
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
        el->f_ct_.store(~static_cast<flag_t>(cur_ct), std::memory_order_release);
        while (1) {
            auto cac_ct = el->f_ct_.load(std::memory_order_acquire);
            if (cur_ct != wt_.load(std::memory_order_acquire)) {
                return true;
            }
            if ((~cac_ct) != cur_ct) {
                return true;
            }
            if (!el->f_ct_.compare_exchange_strong(cac_ct, 0, std::memory_order_relaxed)) {
                return true;
            }
            wt_.store(nxt_ct, std::memory_order_release);
            cur_ct = nxt_ct;
            nxt_ct = cur_ct + 1;
            el = elems + circ::index_of(cur_ct);
        }
        return true;
    }

    template <typename W, typename F, typename E>
    bool force_push(W* wrapper, F&& f, E* elems) {
        return push(wrapper, std::forward<F>(f), elems); /* TBD */
    }

    template <typename W, typename F, template <std::size_t, std::size_t> class E, std::size_t DS, std::size_t AS>
    bool pop(W* /*wrapper*/, circ::u2_t& /*cur*/, F&& f, E<DS, AS>* elems) {
        byte_t buff[DS];
        for (unsigned k = 0;;) {
            auto cur_rd = rd_.load(std::memory_order_relaxed);
            auto cur_wt = wt_.load(std::memory_order_acquire);
            auto id_rd  = circ::index_of(cur_rd);
            auto id_wt  = circ::index_of(cur_wt);
            if (id_rd == id_wt) {
                auto* el = elems + id_wt;
                auto cac_ct = el->f_ct_.load(std::memory_order_acquire);
                if ((~cac_ct) != cur_wt) {
                    return false; // empty
                }
                if (el->f_ct_.compare_exchange_weak(cac_ct, 0, std::memory_order_relaxed)) {
                    wt_.store(cur_wt + 1, std::memory_order_release);
                }
                k = 0;
            }
            else {
                std::memcpy(buff, &(elems[circ::index_of(cur_rd)].data_), sizeof(buff));
                if (rd_.compare_exchange_weak(cur_rd, cur_rd + 1, std::memory_order_release)) {
                    std::forward<F>(f)(buff);
                    return true;
                }
                ipc::yield(k);
            }
        }
    }
};

template <>
struct prod_cons_impl<wr<relat::single, relat::multi, trans::broadcast>> {

    using rc_t = std::size_t;

    template <std::size_t DataSize, std::size_t AlignSize>
    struct elem_t {
        std::aligned_storage_t<DataSize, AlignSize> data_ {};
        std::atomic<rc_t> rc_ { 0 }; // read-counter
    };

    alignas(detail::cache_line_size) std::atomic<circ::u2_t> wt_; // write index

    circ::u2_t cursor() const noexcept {
        return wt_.load(std::memory_order_acquire);
    }

    template <typename W, typename F, typename E>
    bool push(W* wrapper, F&& f, E* elems) {
        E* el;
        for (unsigned k = 0;;) {
            auto cc = wrapper->conn_count(std::memory_order_relaxed);
            if (cc == 0) return false; // no reader
            el = elems + circ::index_of(wt_.load(std::memory_order_acquire));
            // check all consumers have finished reading this element
            auto cur_rc = el->rc_.load(std::memory_order_acquire);
            if (cur_rc) {
                return false; // full
            }
            // cur_rc should be 0 here
            if (el->rc_.compare_exchange_weak(
                        cur_rc, static_cast<rc_t>(cc), std::memory_order_release)) {
                wrapper->clear_dis_flag(std::memory_order_relaxed);
                break;
            }
            ipc::yield(k);
        }
        std::forward<F>(f)(&(el->data_));
        wt_.fetch_add(1, std::memory_order_release);
        return true;
    }

    template <typename W, typename F, typename E>
    bool force_push(W* wrapper, F&& f, E* elems) {
        E* el;
        for (unsigned k = 0;;) {
            auto cc = wrapper->conn_count(std::memory_order_relaxed);
            if (cc == 0) return false; // no reader
            el = elems + circ::index_of(wt_.load(std::memory_order_acquire));
            // check all consumers have finished reading this element
            auto cur_rc = el->rc_.load(std::memory_order_acquire);
            if (cur_rc) {
                wrapper->try_disconnect(); // try disconnect a reader
                cc = wrapper->conn_count(std::memory_order_relaxed);
                if (cc == 0) return false; // no reader
            }
            // just compare & exchange
            if (el->rc_.compare_exchange_weak(
                        cur_rc, static_cast<rc_t>(cc), std::memory_order_release)) {
                break;
            }
            ipc::yield(k);
        }
        std::forward<F>(f)(&(el->data_));
        wt_.fetch_add(1, std::memory_order_release);
        return true;
    }

    template <typename W, typename F, typename E>
    bool pop(W* /*wrapper*/, circ::u2_t& cur, F&& f, E* elems) {
        if (cur == cursor()) return false; // acquire
        auto* el = elems + circ::index_of(cur++);
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
struct prod_cons_impl<wr<relat::multi , relat::multi, trans::broadcast>> {

    using rc_t   = std::uint64_t;
    using flag_t = std::uint64_t;

    enum : rc_t {
        rc_mask = 0x00000000ffffffffull,
        rc_incr = 0x0000000100000000ull
    };

    template <std::size_t DataSize, std::size_t AlignSize>
    struct elem_t {
        std::aligned_storage_t<DataSize, AlignSize> data_ {};
        std::atomic<rc_t  > rc_   { 0 }; // read-counter
        std::atomic<flag_t> f_ct_ { 0 }; // commit flag
    };

    alignas(detail::cache_line_size) std::atomic<circ::u2_t> ct_; // commit index

    circ::u2_t cursor() const noexcept {
        return ct_.load(std::memory_order_acquire);
    }

    template <typename W, typename F, typename E>
    bool push(W* wrapper, F&& f, E* elems) {
        E* el;
        circ::u2_t cur_ct;
        for (unsigned k = 0;;) {
            auto cc = wrapper->conn_count(std::memory_order_relaxed);
            if (cc == 0) return false; // no reader
            el = elems + circ::index_of(cur_ct = ct_.load(std::memory_order_relaxed));
            // check all consumers have finished reading this element
            auto cur_rc = el->rc_.load(std::memory_order_acquire);
            if (cur_rc & rc_mask) {
                return false; // full
            }
            auto cur_fl = el->f_ct_.load(std::memory_order_acquire);
            if ((cur_fl != cur_ct) && cur_fl) {
                return false; // full
            }
            // (cur_rc & rc_mask) should be 0 here
            if (el->rc_.compare_exchange_weak(
                        cur_rc, static_cast<rc_t>(cc) | ((cur_rc & ~rc_mask) + rc_incr), std::memory_order_release)) {
                wrapper->clear_dis_flag(std::memory_order_relaxed);
                break;
            }
            ipc::yield(k);
        }
        // only one thread/process would touch here at one time
        ct_.store(cur_ct + 1, std::memory_order_release);
        std::forward<F>(f)(&(el->data_));
        // set flag & try update wt
        el->f_ct_.store(~static_cast<flag_t>(cur_ct), std::memory_order_release);
        return true;
    }

    template <typename W, typename F, typename E>
    bool force_push(W* wrapper, F&& f, E* elems) {
        E* el;
        circ::u2_t cur_ct;
        for (unsigned k = 0;;) {
            auto cc = wrapper->conn_count(std::memory_order_relaxed);
            if (cc == 0) return false; // no reader
            el = elems + circ::index_of(cur_ct = ct_.load(std::memory_order_relaxed));
            // check all consumers have finished reading this element
            auto cur_rc = el->rc_.load(std::memory_order_acquire);
            ipc::log("force_push: k = %d, cc = %zd, rc = %zd\n", k, cc, (cur_rc & rc_mask));
            if (cur_rc & rc_mask) {
                wrapper->try_disconnect(); // try disconnect a reader
                cc = wrapper->conn_count(std::memory_order_relaxed);
                if (cc == 0) return false; // no reader
            }
            // just compare & exchange
            if (el->rc_.compare_exchange_weak(
                        cur_rc, static_cast<rc_t>(cc) | ((cur_rc & ~rc_mask) + rc_incr), std::memory_order_release)) {
                break;
            }
            ipc::yield(k);
        }
        // only one thread/process would touch here at one time
        ct_.store(cur_ct + 1, std::memory_order_release);
        std::forward<F>(f)(&(el->data_));
        // set flag & try update wt
        el->f_ct_.store(~static_cast<flag_t>(cur_ct), std::memory_order_release);
        return true;
    }

    template <typename W, typename F, typename E, std::size_t N>
    bool pop(W* /*wrapper*/, circ::u2_t& cur, F&& f, E(& elems)[N]) {
        auto* el = elems + circ::index_of(cur);
        auto cur_fl = el->f_ct_.load(std::memory_order_acquire);
        if (cur_fl != ~static_cast<flag_t>(cur)) {
            return false; // empty
        }
        ++cur;
        std::forward<F>(f)(&(el->data_));
        for (unsigned k = 0;;) {
            auto cur_rc = el->rc_.load(std::memory_order_acquire);
            switch (cur_rc & rc_mask) {
            case 0:
                el->f_ct_.store(cur + N - 1, std::memory_order_release);
                return true;
            case 1:
                el->f_ct_.store(cur + N - 1, std::memory_order_release);
                IPC_FALLTHROUGH_;
            default:
                if (el->rc_.compare_exchange_weak(
                            cur_rc, cur_rc + rc_incr - 1, std::memory_order_release)) {
                    return true;
                }
                break;
            }
            ipc::yield(k);
        }
    }
};

} // namespace ipc
