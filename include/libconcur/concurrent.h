/**
 * \file libconcur/concurrent.h
 * \author mutouyun (orz@orzz.org)
 * \brief Define different policies for concurrent queue.
 * \date 2022-11-07
 */
#pragma once

#include <type_traits>
#include <atomic>
#include <array>
#include <tuple>
#include <cstdint>
#include <climits>

#include "libimp/span.h"
#include "libimp/generic.h"

#include "libconcur/def.h"
#include "libconcur/element.h"

LIBCONCUR_NAMESPACE_BEG_

/// \typedef The queue index type.
using index_t = std::uint32_t;

namespace detail_concurrent {

template <typename T, typename = void>
struct has_header : std::false_type {};
template <typename T>
struct has_header<T, ::LIBIMP::void_t<typename T::header>>
  : std::true_type {};

template <typename T, typename = void>
struct has_header_impl : std::false_type {};
template <typename T>
struct has_header_impl<T, ::LIBIMP::void_t<typename T::header_impl>>
  : std::true_type {};

template <typename T, typename = void>
struct has_context : std::false_type {};
template <typename T>
struct has_context<T, ::LIBIMP::void_t<typename T::context>>
  : std::true_type {};

template <typename T, typename = void>
struct has_context_impl : std::false_type {};
template <typename T>
struct has_context_impl<T, ::LIBIMP::void_t<typename T::context_impl>>
  : std::true_type {};

template <typename T, bool = has_header<T>::value
                    , bool = has_header_impl<T>::value>
struct traits_header {
  struct header {};
};

template <typename T, bool HasH>
struct traits_header<T, true, HasH> {
  using header = typename T::header;
};

template <typename T>
struct traits_header<T, false, true> {
  using header = typename T::header_impl;
};

template <typename T, bool = has_context<T>::value
                    , bool = has_context_impl<T>::value>
struct traits_context {
  struct context {};
};

template <typename T, bool HasH>
struct traits_context<T, true, HasH> {
  using context = typename T::context;
};

template <typename T>
struct traits_context<T, false, true> {
  using context = typename T::context_impl;
};

} // namespace detail_concurrent

/// \typedef Utility template for extracting object type internal traits.
template <typename T>
struct traits : detail_concurrent::traits_header<T>
              , detail_concurrent::traits_context<T> {};

/// \brief Multiplicity of the relationship.
namespace relation {

class single {};
class multi  {};

} // namespace relation

/// \brief Transmission mode
namespace trans {

/// \brief In this transmission mode, the message transmission is similar to a queue. 
/// When the receiving side is not timely enough, the sending side will be unable to write data.
class unicast {};

/// \brief In this transmission mode, the message will be perceived by each receiver. 
/// When the receiving side is not timely enough, the sending side will overwrite unread data.
class broadcast {};

} // namespace trans

/// \brief Determines whether type T can be implicitly converted to type U.
template <typename T, typename U>
using convertible = std::enable_if_t<std::is_convertible<T *, U *>::value, bool>;

/// \brief Check whether the elems header type is valid.
template <typename T, typename = void>
struct is_elems_header : std::false_type {};
template <typename T>
struct is_elems_header<T, ::LIBIMP::void_t<
    decltype(std::declval<index_t>() % std::declval<T>().circ_size), 
    std::enable_if_t<std::is_convertible<decltype(std::declval<T>().valid()), bool>::value>>>
  : std::true_type {};

/// \brief Utility template for verifying elems header type.
template <typename T>
using verify_elems_header = std::enable_if_t<is_elems_header<T>::value, bool>;

/**
 * \brief Calculate the corresponding queue position modulo the index value.
 * 
 * \tparam H a elems header type
 * \param hdr a elems header object
 * \param idx a elems array index
 * \return index_t - a corresponding queue position
 */
template <typename H, verify_elems_header<H> = true>
constexpr index_t trunc_index(H const &hdr, index_t idx) noexcept {
  // `circ_size == 2^N` => `idx & (circ_size - 1)`
  return hdr.valid() ? (idx % hdr.circ_size) : 0;
}

/// \brief Producer algorithm implementation.
template <typename TransModT, typename RelationT>
struct producer;

/// \brief Consumer algorithm implementation.
template <typename TransModT, typename RelationT>
struct consumer;

/**
 * \brief Algorithm definition under unicast transmission model.
 * 
 * \ref A bounded wait-free(almost) zero-copy MPMC queue written in C++11.
 * Modified from MengRao/WFMPMC.
 * Copyright (c) 2018. Meng Rao (https://github.com/MengRao/WFMPMC).
*/

/// \brief Single-write producer model implementation.
template <>
struct producer<trans::unicast, relation::single> {

  struct header_impl {
    index_t w_idx {0}; ///< write index
    private: padding<decltype(w_idx)> ___;
  };

  template <typename T, typename H, typename C, typename U, 
            verify_elems_header<H> = true,
            convertible<H, header_impl> = true>
  static bool enqueue(::LIBIMP::span<element<T>> elems, H &hdr, C &/*ctx*/, U &&src) noexcept {
    auto w_idx = hdr.w_idx;
    auto w_cur = trunc_index(hdr, w_idx);
    auto &elem = elems[w_cur];
    auto f_ct  = elem.get_flag();
    // Verify index.
    if ((f_ct != state::invalid_value) && 
        (f_ct != w_idx)) {
      return false; // full
    }
    // Get a valid index and iterate backwards.
    hdr.w_idx += 1;
    // Set data & flag.
    elem.set_data(std::forward<U>(src));
    elem.set_flag(static_cast<index_t>(~w_idx));
    return true;
  }
};

/// \brief Multi-write producer model implementation.
template <>
struct producer<trans::unicast, relation::multi> {

  struct header_impl {
    std::atomic<index_t> w_idx {0}; ///< write index
    private: padding<decltype(w_idx)> ___;
  };

  template <typename T, typename H, typename C, typename U, 
            verify_elems_header<H> = true,
            convertible<H, header_impl> = true>
  static bool enqueue(::LIBIMP::span<element<T>> elems, H &hdr, C &/*ctx*/, U &&src) noexcept {
    auto w_idx = hdr.w_idx.load(std::memory_order_acquire);
    for (;;) {
      auto w_cur = trunc_index(hdr, w_idx);
      auto &elem = elems[w_cur];
      auto f_ct  = elem.get_flag();
      // Verify index.
      if ((f_ct != state::invalid_value) && 
          (f_ct != w_idx)) {
        return false; // full
      }
      // Get a valid index and iterate backwards.
      if (!hdr.w_idx.compare_exchange_weak(w_idx, w_idx + 1, std::memory_order_acq_rel)) {
        continue;
      }
      // Set data & flag.
      elem.set_data(std::forward<U>(src));
      elem.set_flag(static_cast<index_t>(~w_idx));
      return true;
    }
  }
};

/// \brief Single-read consumer model implementation.
template <>
struct consumer<trans::unicast, relation::single> {

  struct header_impl {
    index_t r_idx {0}; ///< read index
    private: padding<decltype(r_idx)> ___;
  };

  template <typename T, typename H, typename C, typename U, 
            verify_elems_header<H> = true,
            convertible<H, header_impl> = true,
            std::enable_if_t<std::is_nothrow_move_assignable<U>::value, bool> = true>
  static bool dequeue(::LIBIMP::span<element<T>> elems, H &hdr, C &/*ctx*/, U &des) noexcept {
    auto r_idx = hdr.r_idx;
    auto r_cur = trunc_index(hdr, r_idx);
    auto &elem = elems[r_cur];
    auto f_ct  = elem.get_flag();
    // Verify index.
    if (f_ct != static_cast<index_t>(~r_idx)) {
      return false; // empty
    }
    // Get a valid index and iterate backwards.
    hdr.r_idx += 1;
    // Get data & set flag.
    des = LIBCONCUR::get(std::move(elem));
    elem.set_flag(r_idx + hdr.circ_size);
    return true;
  }
};

/// \brief Multi-read consumer model implementation.
template <>
struct consumer<trans::unicast, relation::multi> {

  struct header_impl {
    std::atomic<index_t> r_idx {0}; ///< read index
    private: padding<decltype(r_idx)> ___;
  };

  template <typename T, typename H, typename C, typename U, 
            verify_elems_header<H> = true,
            convertible<H, header_impl> = true,
            std::enable_if_t<std::is_nothrow_move_assignable<U>::value, bool> = true>
  static bool dequeue(::LIBIMP::span<element<T>> elems, H &hdr, C &/*ctx*/, U &des) noexcept {
    auto r_idx = hdr.r_idx.load(std::memory_order_acquire);
    for (;;) {
      auto r_cur = trunc_index(hdr, r_idx);
      auto &elem = elems[r_cur];
      auto f_ct  = elem.get_flag();
      // Verify index.
      if (f_ct != static_cast<index_t>(~r_idx)) {
        return false; // empty
      }
      // Get a valid index and iterate backwards.
      if (!hdr.r_idx.compare_exchange_weak(r_idx, r_idx + 1, std::memory_order_acq_rel)) {
        continue;
      }
      // Get data & set flag.
      des = LIBCONCUR::get(std::move(elem));
      elem.set_flag(r_idx + hdr.circ_size);
      return true;
    }
  }
};

/**
 * \brief Algorithm definition under broadcast transmission model.
*/

/// \brief Single-write producer model implementation.
template <>
struct producer<trans::broadcast, relation::single> {

  struct header_impl {
    std::atomic<index_t> w_idx {0}; ///< write index
    std::atomic<index_t> w_beg {0}; ///< write begin index
    private: padding<std::tuple<decltype(w_idx), decltype(w_beg)>> ___;

  public:
    void get(index_t &idx, index_t &beg) const noexcept {
      idx = w_idx.load(std::memory_order_relaxed);
      beg = w_beg.load(std::memory_order_relaxed);
    }
  };

  template <typename T, typename H, typename C, typename U, 
            verify_elems_header<H> = true,
            convertible<H, header_impl> = true>
  static bool enqueue(::LIBIMP::span<element<T>> elems, H &hdr, C &/*ctx*/, U &&src) noexcept {
    auto w_idx = hdr.w_idx.load(std::memory_order_acquire);
    auto w_beg = hdr.w_beg.load(std::memory_order_relaxed);
    auto w_cur = trunc_index(hdr, w_idx);
    auto &elem = elems[w_cur];
    // Move the queue head index.
    if (w_beg + hdr.circ_size <= w_idx) {
      hdr.w_beg.fetch_add(1, std::memory_order_release);
    }
    // Get a valid index and iterate backwards.
    hdr.w_idx.fetch_add(1, std::memory_order_release);
    // Set data & flag.
    elem.set_flag(w_idx | state::enqueue_mask);
    elem.set_data(std::forward<U>(src));  // Here should not be interrupted.
    elem.set_flag(w_idx | state::commit_mask);
    return true;
  }
};

/// \brief Multi-write producer model implementation.
template <>
struct producer<trans::broadcast, relation::multi> {

  struct header_impl {
    std::atomic<state::flag_t> w_flags {0}; ///< write flags, combined current and starting index.
    private: padding<decltype(w_flags)> ___;

  public:
    void get(index_t &idx, index_t &beg) const noexcept {
      auto w_flags = this->w_flags.load(std::memory_order_relaxed);
      idx = get_index(w_flags);
      beg = get_begin(w_flags);
    }
  };

  template <typename T, typename H, typename C, typename U, 
            verify_elems_header<H> = true,
            convertible<H, header_impl> = true>
  static bool enqueue(::LIBIMP::span<element<T>> elems, H &hdr, C &/*ctx*/, U &&src) noexcept {
    auto w_flags = hdr.w_flags.load(std::memory_order_acquire);
    index_t w_idx;
    for (;;) {
      w_idx = get_index(w_flags);
      auto w_beg = get_begin(w_flags);
      // Move the queue head index.
      if (w_beg + hdr.circ_size <= w_idx) {
        w_beg += 1;
      }
      // Update flags.
      auto n_flags = make_flags(w_idx + 1/*iterate backwards*/, w_beg);
      if (hdr.w_flags.compare_exchange_weak(w_flags, n_flags, std::memory_order_acq_rel)) {
        break;
      }
    }
    // Get element.
    auto w_cur = trunc_index(hdr, w_idx);
    auto &elem = elems[w_cur];
    // Set data & flag.
    elem.set_flag(w_idx | state::enqueue_mask);
    elem.set_data(std::forward<U>(src));  // Here should not be interrupted.
    elem.set_flag(w_idx | state::commit_mask);
    return true;
  }

private:
  friend struct producer::header_impl;

  constexpr static index_t get_index(state::flag_t flags) noexcept {
    return index_t(flags);
  }

  constexpr static index_t get_begin(state::flag_t flags) noexcept {
    return index_t(flags >> (sizeof(index_t) * CHAR_BIT));
  }

  constexpr static state::flag_t make_flags(index_t idx, index_t beg) noexcept {
    return state::flag_t(idx) | (state::flag_t(beg) << (sizeof(index_t) * CHAR_BIT));
  }
};

/// \brief Multi-read consumer model implementation.
/// Single-read consumer model is not required.
template <>
struct consumer<trans::broadcast, relation::multi> {

  struct context_impl {
    index_t       r_idx {0};                    ///< read index
    state::flag_t w_lst {state::invalid_value}; ///< last write index
  };

  template <typename T, typename H, typename C, typename U, 
            verify_elems_header<H> = true,
            convertible<C, context_impl> = true,
            std::enable_if_t<std::is_nothrow_copy_assignable<U>::value, bool> = true>
  static bool dequeue(::LIBIMP::span<element<T>> elems, H &hdr, C &ctx, U &des) noexcept {
    index_t w_idx, w_beg;
    hdr.get(w_idx, w_beg);
    // Verify index.
    if (ctx.w_lst == w_idx) {
      return false; // not ready
    }
    // Obtain the queue head index if we need.
    if ((ctx.r_idx <  w_beg) || 
        (ctx.r_idx >= w_beg + hdr.circ_size)) {
      ctx.r_idx = w_beg;
    }
    auto r_cur = trunc_index(hdr, ctx.r_idx);
    auto &elem = elems[r_cur];
    auto f_ct = elem.get_flag();
    if (f_ct == state::invalid_value) {
      return false; // empty
    }
    // Try getting data.
    for (;;) {
      if ((f_ct & state::enqueue_mask) == state::enqueue_mask) {
        return false; // unreadable
      }
      des = LIBCONCUR::get(elem);
      // Correct data can be obtained only if 
      // the elem data is not modified during the getting process.
      if (elem.cas_flag(f_ct, f_ct)) break;
    }
    ctx.w_lst = (f_ct & ~state::enqueue_mask) + 1;
    // Get a valid index and iterate backwards.
    ctx.r_idx += 1;
    return true;
  }
};

/**
 * \brief Producer-consumer implementation.
 * 
 * \tparam TransModT transmission mode (trans::unicast/trans::broadcast)
 * \tparam ProdModT producer relationship model (relation::single/relation::multi)
 * \tparam ConsModT consumer relationship model (relation::single/relation::multi)
 */
template <typename TransModT, typename ProdModT, typename ConsModT>
struct prod_cons : producer<TransModT, ProdModT>
                 , consumer<TransModT, ConsModT> {

  /// \brief Mixing producer and consumer header definitions.
  struct header : traits<producer<TransModT, ProdModT>>::header
                , traits<consumer<TransModT, ConsModT>>::header {
    index_t const circ_size;

    header(index_t cs) noexcept
      : circ_size(cs) {}

    template <typename T>
    header(::LIBIMP::span<element<T>> const &elems) noexcept
      : circ_size(static_cast<index_t>(elems.size())) {}

    bool valid() const noexcept {
      // circ_size must be a power of two.
      return (circ_size > 1) && ((circ_size & (circ_size - 1)) == 0);
    }
  };

  /// \brief Mixing producer and consumer context definitions.
  struct context : traits<producer<TransModT, ProdModT>>::context
                 , traits<consumer<TransModT, ConsModT>>::context {};
};

LIBCONCUR_NAMESPACE_END_
