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
#include <cstdint>

#include "libimp/span.h"

#include "libconcur/def.h"
#include "libconcur/element.h"

LIBCONCUR_NAMESPACE_BEG_

/// \typedef The queue index type.
using index_t = std::uint32_t;

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
using is_convertible = std::enable_if_t<std::is_convertible<T *, U *>::value, bool>;

/// \brief Check whether the elems header type is valid.
template <typename T>
using is_elems_header = decltype(
  std::declval<index_t>() % std::declval<T>().circ_size, 
  std::enable_if_t<std::is_convertible<decltype(std::declval<T>().valid()), bool>::value, bool>{});

/**
 * \brief Calculate the corresponding queue position modulo the index value.
 * 
 * \tparam H a elems header type
 * \param hdr a elems header object
 * \param idx a elems array index
 * \return index_t - a corresponding queue position
 */
template <typename H, is_elems_header<H> = true>
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

  template <typename T, typename H, typename U, 
            is_elems_header<H> = true,
            is_convertible<H, header_impl> = true>
  static bool enqueue(::LIBIMP::span<element<T>> elems, H &hdr, U &&src) noexcept {
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

  template <typename T, typename H, typename U, 
            is_elems_header<H> = true,
            is_convertible<H, header_impl> = true>
  static bool enqueue(::LIBIMP::span<element<T>> elems, H &hdr, U &&src) noexcept {
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

  template <typename T, typename H, typename U, 
            is_elems_header<H> = true,
            is_convertible<H, header_impl> = true>
  static bool dequeue(::LIBIMP::span<element<T>> elems, H &hdr, U &des) noexcept {
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
    des = LIBCONCUR::get(elem);
    elem.set_flag(r_idx + static_cast<index_t>(elems.size())/*avoid overflow*/);
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

  template <typename T, typename H, typename U, 
            is_elems_header<H> = true,
            is_convertible<H, header_impl> = true>
  static bool dequeue(::LIBIMP::span<element<T>> elems, H &hdr, U &des) noexcept {
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
      des = LIBCONCUR::get(elem);
      elem.set_flag(r_idx + static_cast<index_t>(elems.size())/*avoid overflow*/);
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
};

/// \brief Multi-write producer model implementation.
template <>
struct producer<trans::broadcast, relation::multi> {
};

/// \brief Multi-read consumer model implementation.
/// Single-read consumer model is not required.
template <>
struct consumer<trans::broadcast, relation::multi> {
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
  struct header : producer<TransModT, ProdModT>::header_impl
                , consumer<TransModT, ConsModT>::header_impl {
    index_t const circ_size;

    constexpr header(index_t cs) noexcept
      : circ_size(cs) {}

    template <typename T>
    constexpr header(::LIBIMP::span<element<T>> elems) noexcept
      : circ_size(static_cast<index_t>(elems.size())) {}

    constexpr bool valid() const noexcept {
      // circ_size must be a power of two.
      return (circ_size > 1) && ((circ_size & (circ_size - 1)) == 0);
    }
  };
};

LIBCONCUR_NAMESPACE_END_
