#pragma once

#include <atomic>
#include <utility>
#include <cstring>
#include <type_traits>
#include <cstdint>

#include "libipc/def.h"

#include "libipc/platform/detail.h"
#include "libipc/circ/elem_def.h"
#include "libipc/utility/log.h"
#include "libipc/utility/utility.h"

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

    alignas(cache_line_size) std::atomic<circ::u2_t> rd_; // read index
    alignas(cache_line_size) std::atomic<circ::u2_t> wt_; // write index

    constexpr circ::u2_t cursor() const noexcept {
        return 0;
    }

    template <typename F, typename E>
    bool push(F&& f, E* elems) {
        auto cur_wt = circ::index_of(wt_.load(std::memory_order_relaxed));
        if (cur_wt == circ::index_of(rd_.load(std::memory_order_acquire) - 1)) {
            return false; // full
        }
        std::forward<F>(f)(&(elems[cur_wt].data_));
        wt_.fetch_add(1, std::memory_order_release);
        return true;
    }

    template <typename F, typename E>
    bool pop(circ::u2_t& /*cur*/, F&& f, E* elems) {
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

    template <typename F, 
              template <std::size_t, std::size_t> class E, std::size_t DS, std::size_t AS>
    bool pop(circ::u2_t& /*cur*/, F&& f, E<DS, AS>* elems) {
        for (unsigned k = 0;;) {
            auto cur_rd = rd_.load(std::memory_order_relaxed);
            if (circ::index_of(cur_rd) ==
                circ::index_of(wt_.load(std::memory_order_acquire))) {
                return false; // empty
            }
            if (rd_.compare_exchange_weak(cur_rd, cur_rd + 1, std::memory_order_release)) {
                std::forward<F>(f)(&(elems[circ::index_of(cur_rd)].data_));
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

    alignas(cache_line_size) std::atomic<circ::u2_t> ct_; // commit index

    template <typename F, typename E>
    bool push(F&& f, E* elems) {
        circ::u2_t cur_ct, nxt_ct;
        for (unsigned k = 0;;) {
            cur_ct = ct_.load(std::memory_order_relaxed);
            if (circ::index_of(nxt_ct = cur_ct + 1) ==
                circ::index_of(rd_.load(std::memory_order_acquire))) {
                return false; // full
            }
            if (ct_.compare_exchange_weak(cur_ct, nxt_ct, std::memory_order_acq_rel)) {
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
            if (cur_ct != wt_.load(std::memory_order_relaxed)) {
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

    template <typename F, 
              template <std::size_t, std::size_t> class E, std::size_t DS, std::size_t AS>
    bool pop(circ::u2_t& /*cur*/, F&& f, E<DS, AS>* elems) {
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
            else if (rd_.compare_exchange_weak(cur_rd, cur_rd + 1, std::memory_order_release)) {
                std::forward<F>(f)(&(elems[id_rd].data_));
                return true;
            }
            ipc::yield(k);
        }
    }
};

template <>
struct prod_cons_impl<wr<relat::single, relat::multi, trans::broadcast>> {

    using flag_t = std::uint64_t;

    template <std::size_t DataSize, std::size_t AlignSize>
    struct elem_t {
        std::aligned_storage_t<DataSize, AlignSize> data_ {};
        std::atomic<flag_t> f_rc_ { 0 }; // read-flag
    };

    alignas(cache_line_size) std::atomic<circ::u2_t> wt_;   // write index

    circ::u2_t cursor() const noexcept {
        return wt_.load(std::memory_order_acquire);
    }

    template <typename F, typename E>
    bool push(F&& f, E* elems) {
        E* el = elems + circ::index_of(wt_.load(std::memory_order_relaxed));
        auto cur_rc = el->f_rc_.exchange(~0ull, std::memory_order_acq_rel);
        // check for consumers to read this element
        if (cur_rc != 0) {
            return false; // full
        }
        std::forward<F>(f)(&(el->data_));
        wt_.fetch_add(1, std::memory_order_release);
        return true;
    }

    template <typename F, typename E>
    bool pop(circ::u2_t& cur, F&& f, E* elems) {
        if (cur == cursor()) return false; // empty
        E* el = elems + circ::index_of(cur++);
        std::forward<F>(f)(&(el->data_));
        el->f_rc_.store(0, std::memory_order_release);
        return true;
    }
};

template <>
struct prod_cons_impl<wr<relat::multi, relat::multi, trans::broadcast>> {

    using flag_t = std::uint64_t;

    enum : flag_t {
        pushing = 1ull,
        pushed  = ~0ull,
        popped  = 0ull,
    };

    template <std::size_t DataSize, std::size_t AlignSize>
    struct elem_t {
        std::aligned_storage_t<DataSize, AlignSize> data_ {};
        std::atomic<flag_t> f_rc_ { 0 }; // read-flag
        std::atomic<flag_t> f_ct_ { 0 }; // commit-flag
    };

    alignas(cache_line_size) std::atomic<circ::u2_t> ct_;   // commit index
    alignas(cache_line_size) std::atomic<circ::u2_t> wt_;   // write index

    circ::u2_t cursor() const noexcept {
        return wt_.load(std::memory_order_acquire);
    }

    template <typename F, typename E>
    bool push(F&& f, E* elems) {
        E* el;
        circ::u2_t cur_ct, nxt_ct;
        for (unsigned k = 0;;) {
            auto cac_ct = ct_.load(std::memory_order_relaxed);
            cur_ct = cac_ct;
            nxt_ct = cur_ct + 1;
            el = elems + circ::index_of(cac_ct);
            for (unsigned k = 0;;) {
                auto cur_rc = el->f_rc_.load(std::memory_order_acquire);
                switch (cur_rc) {
                // helper
                case pushing:
                    ct_.compare_exchange_strong(cac_ct, nxt_ct, std::memory_order_release);
                    goto try_next;
                // full
                case pushed:
                    return false;
                // writable
                default:
                    break;
                }
                if (el->f_rc_.compare_exchange_weak(cur_rc, pushing, std::memory_order_release)) {
                    break;
                }
                ipc::yield(k);
            }
            ct_.compare_exchange_strong(cac_ct, nxt_ct, std::memory_order_relaxed);
            el->f_rc_.store(pushed, std::memory_order_relaxed);
            std::atomic_thread_fence(std::memory_order_release);
            break;
        try_next:
            ipc::yield(k);
        }
        std::forward<F>(f)(&(el->data_));
        // set flag & try update wt
        el->f_ct_.store(~static_cast<flag_t>(cur_ct), std::memory_order_release);
        while (1) {
            auto cac_ct = el->f_ct_.load(std::memory_order_acquire);
            if (cur_ct != wt_.load(std::memory_order_relaxed)) {
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

    template <typename F, typename E, std::size_t N>
    bool pop(circ::u2_t& cur, F&& f, E(& elems)[N]) {
        for (unsigned k = 0;;) {
            auto cur_wt = wt_.load(std::memory_order_acquire);
            auto id_rd  = circ::index_of(cur);
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
                ++cur;
                auto* el = elems + id_rd;
                std::forward<F>(f)(&(el->data_));
                el->f_rc_.store(popped, std::memory_order_release);
                return true;
            }
            ipc::yield(k);
        }
    }
};

} // namespace ipc
