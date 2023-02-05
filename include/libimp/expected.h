/**
 * \file libimp/expected.h
 * \author mutouyun (orz@orzz.org)
 * \brief Provides a way to store either of two values.
 * \date 2023-02-05
 */
#pragma once

#include <type_traits>
#include <utility>  // std::exchange
#include <array>
#include <memory>   // std::addressof
#include <cstddef>  // std::nullptr_t

#include "libimp/def.h"
#include "libimp/construct.h"
#include "libimp/generic.h"
#include "libimp/byte.h"

LIBIMP_NAMESPACE_BEG_

/**
 * \brief In-place construction tag for unexpected value in expected.
 * \see https://en.cppreference.com/w/cpp/utility/expected/unexpect_t
 */
struct unexpected_t {
  explicit unexpected_t() = default;
};
constexpr unexpected_t unexpected {};

/**
 * \class template <typename T, typename E> expected
 * \brief Provides a way to store either of two values.
 * \see https://en.cppreference.com/w/cpp/utility/expected
 * \tparam T - the type of the expected value.
 * \tparam E - the type of the unexpected value.
 */
template <typename T, typename E>
class expected;

namespace detail_expected {

template <typename T, typename E>
struct data_union {
  union {
    T value_;  ///< the expected value
    E error_;  ///< the unexpected value
  };

  data_union(data_union const &) = delete;
  data_union &operator=(data_union const &) = delete;

  data_union(std::nullptr_t) noexcept {}
  ~data_union() {}

  template <typename... A>
  data_union(in_place_t, A &&...args) : value_{std::forward<A>(args)...} {}
  template <typename... A>
  data_union(unexpected_t, A &&...args) : error_{std::forward<A>(args)...} {}

  void destruct_value() noexcept { destroy(&value_); }
  void destruct_error() noexcept { destroy(&error_); }
};

template <typename E>
struct data_union<void, E> {
  alignas(E) std::array<byte, sizeof(E)> error_;  ///< the unexpected value

  data_union(data_union const &) = delete;
  data_union &operator=(data_union const &) = delete;

  data_union(std::nullptr_t) noexcept {}

  template <typename... A>
  data_union(in_place_t, A &&...) noexcept {}
  template <typename... A>
  data_union(unexpected_t, A &&...args) {
    construct<E>(&error_, std::forward<A>(args)...);
  }

  void destruct_value() noexcept {}
  void destruct_error() noexcept { destroy(reinterpret_cast<E *>(&error_)); }
};

template <typename T, typename E>
auto destruct(bool /*has_value*/, data_union<T, E> &/*data*/) noexcept
  -> typename std::enable_if<std::is_trivially_destructible<T>::value &&
                             std::is_trivially_destructible<E>::value>::type {
  // Do nothing.
}

template <typename T, typename E>
auto destruct(bool has_value, data_union<T, E> &data) noexcept
  -> typename std::enable_if<!std::is_trivially_destructible<T>::value &&
                              std::is_trivially_destructible<E>::value>::type {
  if (has_value) data.destruct_value();
}

template <typename T, typename E>
auto destruct(bool has_value, data_union<T, E> &data) noexcept
  -> typename std::enable_if< std::is_trivially_destructible<T>::value &&
                             !std::is_trivially_destructible<E>::value>::type {
  if (!has_value) data.destruct_error();
}

template <typename T, typename E>
auto destruct(bool has_value, data_union<T, E> &data) noexcept
  -> typename std::enable_if<!std::is_trivially_destructible<T>::value &&
                             !std::is_trivially_destructible<E>::value>::type {
  if (has_value) {
    data.destruct_value();
  } else {
    data.destruct_error();
  }
}

template <typename S, typename T, typename E>
struct value_getter : data_union<T, E> {
  using data_union<T, E>::data_union;

  value_getter(S const &other) : data_union<T, E>(nullptr) {
    if (bool(static_cast<S &>(other))) {
      construct<data_union<T, E>>(this, in_place, other.value_);
    } else {
      construct<data_union<T, E>>(this, unexpected, other.error_);
    }
  }

  value_getter(S &&other) : data_union<T, E>(nullptr) {
    if (bool(static_cast<S &>(other))) {
      construct<data_union<T, E>>(this, in_place, std::move(other.value_));
    } else {
      construct<data_union<T, E>>(this, unexpected, std::move(other.error_));
    }
  }

  T const & value() const &  noexcept { return this->value_; }
  T &       value()       &  noexcept { return this->value_; }
  T const &&value() const && noexcept { return std::move(this->value_); }
  T &&      value()       && noexcept { return std::move(this->value_); }

  T const *operator->() const noexcept { return std::addressof(this->value_); }
  T *      operator->()       noexcept { return std::addressof(this->value_); }

  T const & operator*() const &  noexcept { return this->value_; }
  T &       operator*()       &  noexcept { return this->value_; }
  T const &&operator*() const && noexcept { return std::move(this->value_); }
  T &&      operator*()       && noexcept { return std::move(this->value_); }

  template <typename U>
  T value_or(U &&def) const & {
    return bool(*static_cast<S *>(this)) ? **this : static_cast<T>(std::forward<U>(def));
  }
  template <typename U>
  T value_or(U &&def) && {
    return bool(*static_cast<S *>(this)) ? std::move(**this) : static_cast<T>(std::forward<U>(def));
  }

  template <typename... A>
  T &emplace(A &&...args) {
    static_cast<S *>(this)->reconstruct(in_place, std::forward<A>(args)...);
    return this->value_;
  }
};

template <typename S, typename E>
struct value_getter<S, void, E> : data_union<void, E> {
  using data_union<void, E>::data_union;

  value_getter(S const &other) : data_union<T, E>(nullptr) {
    if (bool(static_cast<S &>(other))) {
      construct<data_union<T, E>>(this, in_place);
    } else {
      construct<data_union<T, E>>(this, unexpected, other.error_);
    }
  }

  value_getter(S &&other) : data_union<T, E>(nullptr) {
    if (bool(static_cast<S &>(other))) {
      construct<data_union<T, E>>(this, in_place);
    } else {
      construct<data_union<T, E>>(this, unexpected, std::move(other.error_));
    }
  }

  void emplace() noexcept {
    static_cast<S *>(this)->reconstruct(in_place);
  }
};

template <typename T, typename E>
struct storage : value_getter<storage<T, E>, T, E> {
  using getter_t = value_getter<storage<T, E>, T, E>;

  bool has_value_;

  template <typename... A>
  storage(in_place_t, A &&...args)
    : getter_t(in_place, std::forward<A>(args)...)
    , has_value_(true) {}

  template <typename... A>
  storage(unexpected_t, A &&...args)
    : getter_t(unexpected, std::forward<A>(args)...)
    , has_value_(false) {}

  storage(storage const &other)
    : getter_t(other)
    , has_value_(other.has_value_) {}

  storage(storage &&other)
    : getter_t(std::move(other))
    , has_value_(std::exchange(other.has_value_, false)) {}

  bool has_value() const noexcept {
    return has_value_;
  }

  explicit operator bool() const noexcept {
    return this->has_value();
  }

  typename std::add_const<E>::type &error() const & noexcept {
    return this->error_;
  }
  E &error() & noexcept {
    return this->error_;
  }
  typename std::add_const<E>::type &&error() const && noexcept {
    return std::move(this->error_);
  }
  E &&error() && noexcept {
    return std::move(this->error_);
  }

protected:
  friend getter_t;

  template <typename... A>
  void reconstruct(A &&...args) {
    destroy(this);
    construct<storage>(this, std::forward<A>(args)...);
  }
};

} // namespace detail_expected

LIBIMP_NAMESPACE_END_
