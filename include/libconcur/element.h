/**
 * @file libconcur/element.h
 * @author mutouyun (orz@orzz.org)
 * @brief Define concurrent queue element abstraction.
 * @date 2022-11-19
 */
#pragma once

#include <atomic>
#include <utility>
#include <limits>   // std::numeric_limits
#include <cstdint>
#include <exception>

#include "libimp/detect_plat.h"
#include "libimp/log.h"

#include "libconcur/def.h"

LIBCONCUR_NAMESPACE_BEG_
namespace state {

/// @brief The state flag type for the queue element.
using flag_t = std::uint64_t;

enum : flag_t {
  /// @brief The invalid state value.
  invalid_value = (std::numeric_limits<flag_t>::max)(),
};

} // namespace state

template <typename T>
class element {
  /// @brief Committed flag.
  alignas(cache_line_size) std::atomic<state::flag_t> f_ct_;
  /// @brief The user data segment.
  T data_;

  /// @brief Disable copy & move.
  element(element const &) = delete;
  element &operator=(element const &) = delete;

public:
  template <typename... A>
  element(A &&... args) 
    noexcept(noexcept(T{std::forward<A>(args)...}))
    : f_ct_{state::invalid_value}
    , data_{std::forward<A>(args)...} {}

  template <typename U>
  void set_data(U &&src) noexcept {
    LIBIMP_LOG_();
    LIBIMP_TRY {
      data_ = std::forward<U>(src);
    } LIBIMP_CATCH(std::exception const &e) {
      log.error("failed: `data = std::forward<U>(src)`. error = {}", e.what());
    } LIBIMP_CATCH(...) {
      log.error("failed: `data = std::forward<U>(src)`. error = unknown");
    }
  }

  T &&get_data() noexcept {
    return std::move(data_);
  }

  void set_flag(state::flag_t flag) noexcept {
    f_ct_.store(flag, std::memory_order_release);
  }

  state::flag_t get_flag() const noexcept {
    return f_ct_.load(std::memory_order_acquire);
  }
};

LIBCONCUR_NAMESPACE_END_
