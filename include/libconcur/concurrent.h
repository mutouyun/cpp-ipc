/**
 * @file libconcur/concurrent.h
 * @author mutouyun (orz@orzz.org)
 * @brief Define different policies for concurrent queue.
 * @date 2022-11-07
 */
#pragma once

#include <type_traits>
#include <atomic>
#include <cstdint>

#include "libimp/span.h"

#include "libconcur/def.h"
#include "libconcur/element.h"

LIBCONCUR_NAMESPACE_BEG_

/// @brief The queue index type.
using index_t = std::uint32_t;

/// @brief Multiplicity of the relationship.
namespace relation {

class single {};
class multi  {};

} // namespace relation

/// @brief Transmission mode
namespace trans {

class unicast   {};
class broadcast {};

} // namespace trans

/// @brief Determines whether type T can be implicitly converted to type U.
template <typename T, typename U>
using is_convertible = typename std::enable_if<std::is_convertible<T *, U *>::value>::type;

/// @brief Check whether the context type is valid.
template <typename T>
using is_context = decltype(typename std::enable_if<std::is_convertible<decltype(
  std::declval<T>().valid()), bool>::value>::type(), 
  std::declval<index_t>() % std::declval<T>().circ_size);

/**
 * @brief Calculate the corresponding queue position modulo the index value.
 * 
 * @tparam C a context type
 * @param ctx a context object
 * @param idx a context array index
 * @return index_t - a corresponding queue position
 */
template <typename C, typename = is_context<C>>
constexpr index_t trunc_index(C const &ctx, index_t idx) noexcept {
  /// @remark `circ_size == 2^N` => `idx & (circ_size - 1)`
  return ctx.valid() ? (idx % ctx.circ_size) : 0;
}

/// @brief Producer algorithm implementation.
template <typename TransModT, typename RelationT>
struct producer;

/// @brief Consumer algorithm implementation.
template <typename TransModT, typename RelationT>
struct consumer;

/**
 * @brief Algorithm definition under unicast transmission model.
 * 
 * @ref A bounded wait-free(almost) zero-copy MPMC queue written in C++11.
 * Modified from MengRao/WFMPMC.
 * Copyright (c) 2018. Meng Rao (https://github.com/MengRao/WFMPMC).
*/

/// @brief Single-write producer model implementation.
template <>
struct producer<trans::unicast, relation::single> {

  struct context_impl {
    /// @brief Write index.
    alignas(cache_line_size) index_t w_idx {0};
  };

  template <typename T, typename C, typename U, 
            typename = is_context<C>,
            typename = is_convertible<C, context_impl>>
  static bool enqueue(::LIBIMP_::span<element<T>> elems, C &ctx, U &&src) noexcept {
    auto w_idx = ctx.w_idx;
    auto w_cur = trunc_index(ctx, w_idx);
    auto &elem = elems[w_cur];
    auto f_ct  = elem.get_flag();
    /// @remark Verify index.
    if ((f_ct != state::invalid_value) && 
        (f_ct != static_cast<state::flag_t>(w_idx))) {
      return false; // full
    }
    /// @remark Get a valid index and iterate backwards.
    ctx.w_idx += 1;
    /// @remark Set data & flag.
    elem.set_data(std::forward<U>(src));
    elem.set_flag(static_cast<state::flag_t>(~w_idx));
    return true;
  }
};

/// @brief Multi-write producer model implementation.
template <>
struct producer<trans::unicast, relation::multi> {

  struct context_impl {
    /// @brief Write index.
    alignas(cache_line_size) std::atomic<index_t> w_idx {0};
  };

  template <typename T, typename C, typename U, 
            typename = is_context<C>,
            typename = is_convertible<C, context_impl>>
  static bool enqueue(::LIBIMP_::span<element<T>> elems, C &ctx, U &&src) noexcept {
    auto w_idx = ctx.w_idx.load(std::memory_order_acquire);
    for (;;) {
      auto w_cur = trunc_index(ctx, w_idx);
      auto &elem = elems[w_cur];
      auto f_ct  = elem.get_flag();
      /// @remark Verify index.
      if ((f_ct != state::invalid_value) && 
          (f_ct != w_idx)) {
        return false; // full
      }
      /// @remark Get a valid index and iterate backwards.
      if (!ctx.w_idx.compare_exchange_week(w_idx, w_idx + 1, std::memory_order_acq_rel)) {
        continue;
      }
      /// @remark Set data & flag.
      elem.set_data(std::forward<U>(src));
      elem.set_flag(~w_idx);
      return true;
    }
  }
};

/// @brief Single-read consumer model implementation.
template <>
struct consumer<trans::unicast, relation::single> {

  struct context_impl {
    /// @brief Read index.
    alignas(cache_line_size) index_t r_idx {0};
  };

  template <typename T, typename C, typename U, 
            typename = is_context<C>,
            typename = is_convertible<C, context_impl>>
  static bool dequeue(::LIBIMP_::span<element<T>> elems, C &ctx, U &des) noexcept {
    auto r_idx = ctx.r_idx;
    auto r_cur = trunc_index(ctx, r_idx);
    auto &elem = elems[r_cur];
    auto f_ct  = elem.get_flag();
    /// @remark Verify index.
    if (f_ct != ~r_idx) {
      return false; // empty
    }
    /// @remark Get a valid index and iterate backwards.
    ctx.r_idx += 1;
    /// @remark Get data & set flag.
    des = elem.get_data();
    elem.set_flag(r_idx + static_cast<index_t>(elems.size()));
    return true;
  }
};

/// @brief Multi-read consumer model implementation.
template <>
struct consumer<trans::unicast, relation::multi> {

  struct context_impl {
    /// @brief Read index.
    alignas(cache_line_size) std::atomic<index_t> r_idx {0};
  };

  template <typename T, typename C, typename U, 
            typename = is_context<C>,
            typename = is_convertible<C, context_impl>>
  static bool dequeue(::LIBIMP_::span<element<T>> elems, C &ctx, U &des) noexcept {
    auto r_idx = ctx.r_idx.load(std::memory_order_acquire);
    for (;;) {
      auto r_cur = trunc_index(ctx, r_idx);
      auto &elem = elems[r_cur];
      auto f_ct  = elem.get_flag();
      /// @remark Verify index.
      if (f_ct != ~r_idx) {
        return false; // empty
      }
      /// @remark Get a valid index and iterate backwards.
      if (!ctx.r_idx.compare_exchange_week(r_idx, r_idx + 1, std::memory_order_acq_rel)) {
        continue;
      }
      /// @remark Get data & set flag.
      des = elem.get_data();
      elem.set_flag(r_idx + static_cast<index_t>(elems.size()));
      return true;
    }
  }
};

/**
 * @brief Algorithm definition under broadcast transmission model.
*/

/// @brief Single-write producer model implementation.
template <>
struct producer<trans::broadcast, relation::single> {
};

/// @brief Multi-write producer model implementation.
template <>
struct producer<trans::broadcast, relation::multi> {
};

/// @brief Single-read consumer model implementation.
template <>
struct consumer<trans::broadcast, relation::single> {
};

/// @brief Multi-read consumer model implementation.
template <>
struct consumer<trans::broadcast, relation::multi> {
};

/**
 * @brief Producer-consumer implementation.
 * 
 * @tparam TransModT transmission mode (trans::unicast/trans::broadcast)
 * @tparam ProdModT producer relationship model (relation::single/relation::multi)
 * @tparam ConsModT consumer relationship model (relation::single/relation::multi)
 */
template <typename TransModT, typename ProdModT, typename ConsModT>
struct prod_cons : producer<TransModT, ProdModT>
                 , consumer<TransModT, ConsModT> {
  
  /// @brief Mixing producer and consumer context definitions.
  struct context : producer<TransModT, ProdModT>::context_impl
                 , consumer<TransModT, ConsModT>::context_impl {
    index_t const circ_size;

    constexpr context(index_t cs) noexcept
      : circ_size(cs) {}

    constexpr bool valid() const noexcept {
      /// @remark circ_size must be a power of two.
      return (circ_size > 1) && ((circ_size & (circ_size - 1)) == 0);
    }
  };
};

LIBCONCUR_NAMESPACE_END_
