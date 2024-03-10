/**
 * \file libconcur/element.h
 * \author mutouyun (orz@orzz.org)
 * \brief Define concurrent queue element abstraction.
 * \date 2022-11-19
 */
#pragma once

#include <atomic>
#include <utility>
#include <cstdint>
#include <exception>

#include "libimp/detect_plat.h"
#include "libimp/log.h"
#include "libimp/byte.h"
#include "libimp/generic.h"

#include "libconcur/def.h"

LIBCONCUR_NAMESPACE_BEG_
namespace state {

/// \typedef The state flag type for the queue element.
using flag_t = std::uint64_t;

enum : flag_t {
  invalid_value = ~flag_t(0),
  enqueue_mask  = invalid_value << 32,
  commit_mask   = ~flag_t(1) << 32,
};

} // namespace state

/// \brief Define the padding type.
template <typename T>
using padding = std::array<::LIBIMP::byte, (cache_line_size - sizeof(T))>;

/**
 * \class template <typename T> element
 * \brief User-defined type element wrapper.
 * Wrapper for wrapping user-defined types as elements.
 * 
 * @tparam T - User-defined type.
 */
template <typename T>
class element {

  template <typename E>
  friend auto get(E &&elem) noexcept;

  /// \brief Committed flag.
  std::atomic<state::flag_t> f_ct_;
  padding<decltype(f_ct_)> ___;

  /// \brief The user data segment.
  T data_;

  /// \brief Disable copy & move.
  element(element const &) = delete;
  element &operator=(element const &) = delete;

public:
  using value_type = T;

  template <typename... A>
  element(A &&...args) 
    noexcept(noexcept(T{std::forward<A>(args)...}))
    : f_ct_{state::invalid_value}
    , data_{std::forward<A>(args)...} {}

  template <typename U>
  void set_data(U &&src) noexcept {
    LIBIMP_LOG_();
    LIBIMP_TRY {
      data_ = std::forward<U>(src);
    } LIBIMP_CATCH(...) {
      log.error("failed: `data = std::forward<U>(src)`.",
                "\n\texception: ", ::LIBIMP::log::exception_string(std::current_exception()));
    }
  }

  void set_flag(state::flag_t flag, 
                std::memory_order const order = std::memory_order_release) noexcept {
    f_ct_.store(flag, order);
  }

  bool cas_flag(state::flag_t &expected, state::flag_t flag, 
                std::memory_order const order = std::memory_order_acq_rel) noexcept {
    return f_ct_.compare_exchange_weak(expected, flag, order);
  }

  state::flag_t get_flag() const noexcept {
    return f_ct_.load(std::memory_order_acquire);
  }
};

template <typename E>
auto get(E &&elem) noexcept {
  return static_cast<imp::copy_cvref_t<E, typename std::decay<E>::type::value_type>>(elem.data_);
}

LIBCONCUR_NAMESPACE_END_
