/**
 * \file libimp/scope_exit.h
 * \author mutouyun (orz@orzz.org)
 * \brief Execute guard function when the enclosing scope exits.
 * \date 2022-03-01
 */
#pragma once

#include <utility>    // std::forward, std::move
#include <functional> // std::function
#include <type_traits>

#include "libimp/def.h"
#include "libimp/detect_plat.h"

LIBIMP_NAMESPACE_BEG_

template <typename F = std::function<void()>>
class scope_exit {
  F destructor_;
  mutable bool released_;

public:
  template <typename G>
  explicit scope_exit(G &&destructor) noexcept
    : destructor_(std::forward<G>(destructor))
    , released_  (false) {}

  scope_exit(scope_exit &&other) noexcept
    : destructor_(std::move(other.destructor_))
    , released_  (std::exchange(other.released_, true)) /*release rhs*/ {}

  scope_exit &operator=(scope_exit &&other) noexcept {
    destructor_ = std::move(other.destructor_);
    released_   = std::exchange(other.released_, true);
    return *this;
  }

  ~scope_exit() noexcept {
    if (!released_) destructor_();
  }

  void release() const noexcept {
    released_ = true;
  }

  void do_exit() noexcept {
    if (released_) return;
    destructor_();
    released_ = true;
  }

  void swap(scope_exit &other) noexcept {
    std::swap(destructor_, other.destructor_);
    std::swap(released_  , other.released_);
  }
};

/// \brief Creates a scope_exit object.
template <typename F>
auto make_scope_exit(F &&destructor) noexcept {
  return scope_exit<std::decay_t<F>>(std::forward<F>(destructor));
}

namespace detail {

struct scope_exit_helper {
  template <typename F>
  auto operator=(F &&f) const noexcept {
    return make_scope_exit(std::forward<F>(f));
  }
};

} // namespace detail

#define LIBIMP_SCOPE_EXIT($val) \
  LIBIMP_UNUSED auto $val = ::LIBIMP::detail::scope_exit_helper{}

LIBIMP_NAMESPACE_END_
