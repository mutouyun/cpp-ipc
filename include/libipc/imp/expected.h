/**
 * \file libimp/expected.h
 * \author mutouyun (orz@orzz.org)
 * \brief Provides a way to store either of two values.
 */
#pragma once

#include <type_traits>
#include <utility>  // std::exchange
#include <array>
#include <memory>   // std::addressof
#include <cstddef>  // std::nullptr_t

#include "libipc/imp/uninitialized.h"
#include "libipc/imp/generic.h"
#include "libipc/imp/byte.h"

namespace ipc {

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
  using const_value_t = typename std::add_const<T>::type;
  using const_error_t = typename std::add_const<E>::type;

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

  const_value_t & value() const &  noexcept { return value_; }
  T &             value() &        noexcept { return value_; }
  const_value_t &&value() const && noexcept { return std::move(value_); }
  T &&            value() &&       noexcept { return std::move(value_); }

  const_error_t & error() const &  noexcept { return error_; }
  E &             error() &        noexcept { return error_; }
  const_error_t &&error() const && noexcept { return std::move(error_); }
  E &&            error() &&       noexcept { return std::move(error_); }
};

template <typename E>
struct data_union<void, E> {
  using const_error_t = typename std::add_const<E>::type;

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

  const_error_t & error() const &  noexcept { return *reinterpret_cast<E *>(error_.data()); }
  E &             error() &        noexcept { return *reinterpret_cast<E *>(error_.data()); }
  const_error_t &&error() const && noexcept { return std::move(*reinterpret_cast<E *>(error_.data())); }
  E &&            error() &&       noexcept { return std::move(*reinterpret_cast<E *>(error_.data())); }
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

  template <typename U>
  value_getter(U &&other) : data_union<T, E>(nullptr) {
    if (other) {
      construct<data_union<T, E>>(this, in_place, std::forward<U>(other).value());
    } else {
      construct<data_union<T, E>>(this, unexpected, std::forward<U>(other).error());
    }
  }

  T const *operator->() const noexcept { return std::addressof(this->value()); }
  T *      operator->()       noexcept { return std::addressof(this->value()); }

  T const & operator*() const &  noexcept { return this->value(); }
  T &       operator*()       &  noexcept { return this->value(); }
  T const &&operator*() const && noexcept { return std::move(this->value()); }
  T &&      operator*()       && noexcept { return std::move(this->value()); }

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
    return this->value();
  }

  void swap(S &other) {
    if (bool(*static_cast<S *>(this)) && bool(other)) {
      std::swap(this->value(), other.value());
    } else if (!*static_cast<S *>(this) && !other) {
      std::swap(this->error(), other.error());
    } else if (!*static_cast<S *>(this) && bool(other)) {
      E err(std::move(this->error()));
      this->emplace(std::move(other.value()));
      other.reconstruct(unexpected, std::move(err));
    } else /*if (bool(*this) && !other)*/ {
      E err(std::move(other.error()));
      other.emplace(std::move(this->value()));
      static_cast<S *>(this)->reconstruct(unexpected, std::move(err));
    }
  }
};

template <typename S, typename E>
struct value_getter<S, void, E> : data_union<void, E> {
  using data_union<void, E>::data_union;

  template <typename U>
  value_getter(U &&other) : data_union<void, E>(nullptr) {
    if (other) {
      construct<data_union<void, E>>(this, in_place);
    } else {
      construct<data_union<void, E>>(this, unexpected, std::forward<U>(other).error());
    }
  }

  void emplace() noexcept {
    static_cast<S *>(this)->reconstruct(in_place);
  }

  void swap(S &other) {
    if (bool(*static_cast<S *>(this)) && bool(other)) {
      return;
    } else if (!*static_cast<S *>(this) && !other) {
      std::swap(this->error(), other.error());
    } else if (!*static_cast<S *>(this) && bool(other)) {
      E err(std::move(this->error()));
      this->emplace();
      other.reconstruct(unexpected, std::move(err));
    } else /*if (bool(*this) && !other)*/ {
      E err(std::move(other.error()));
      other.emplace();
      static_cast<S *>(this)->reconstruct(unexpected, std::move(err));
    }
  }
};

/**
 * \brief Define the expected storage.
 */
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
    /// After construction, has_value() is equal to other.has_value().
    , has_value_(other.has_value_) {}

  template <typename T_, typename E_>
  storage(storage<T_, E_> const &other)
    : getter_t(other)
    , has_value_(other.has_value_) {}

  template <typename T_, typename E_>
  storage(storage<T_, E_> &&other)
    : getter_t(std::move(other))
    /// After construction, has_value() is equal to other.has_value().
    , has_value_(other.has_value_) {}

  bool has_value() const noexcept {
    return has_value_;
  }

  explicit operator bool() const noexcept {
    return this->has_value();
  }

protected:
  friend getter_t;

  template <typename... A>
  void reconstruct(A &&...args) {
    destroy(this);
    construct<storage>(this, std::forward<A>(args)...);
  }
};

/// \brief The invoke forwarding helper.

template <typename F, typename... A>
auto invoke(F &&f, A &&...args) noexcept(
     noexcept(std::forward<F>(f)(std::forward<A>(args)...)))
  -> decltype(std::forward<F>(f)(std::forward<A>(args)...)) {
  return std::forward<F>(f)(std::forward<A>(args)...);
}

template <typename F, typename... A>
auto invoke(F &&f, A &&...args) noexcept(
     noexcept(std::forward<F>(f)()))
  -> decltype(std::forward<F>(f)()) {
  return std::forward<F>(f)();
}

/// \brief and_then helper.

template <typename E, typename F,
          typename R = decltype(invoke(std::declval<F>(), *std::declval<E>()))>
R and_then(E &&exp, F &&f) {
  static_assert(is_specialized<expected, R>::value, "F must return an `expected`.");
  return bool(exp) ? invoke(std::forward<F>(f), *std::forward<E>(exp))
                   : R(unexpected, std::forward<E>(exp).error());
}

/// \brief or_else helper.

template <typename E, typename F,
          typename R = decltype(invoke(std::declval<F>(), std::declval<E>().error()))>
R or_else(E &&exp, F &&f) {
  static_assert(is_specialized<expected, R>::value, "F must return an `expected`.");
  return bool(exp) ? std::forward<E>(exp)
                   : invoke(std::forward<F>(f), std::forward<E>(exp).error());
}

} // namespace detail_expected

/**
 * \class template <typename T, typename E> expected
 * \brief Provides a way to store either of two values.
 */
template <typename T, typename E>
class expected : public detail_expected::storage<typename std::remove_cv<T>::type, E> {
public:
  using value_type = typename std::remove_cv<T>::type;
  using error_type = E;

  using detail_expected::storage<value_type, E>::storage;

  expected(expected const &) = default;
  expected(expected &&) = default;

  expected()
    : detail_expected::storage<value_type, E>(in_place) {}

  expected &operator=(expected other) {
    this->swap(other);
    return *this;
  }

  // Monadic operations

  template <typename F>
  auto and_then(F &&f) & {
    return detail_expected::and_then(*this, std::forward<F>(f));
  }

  template <typename F>
  auto and_then(F &&f) const & {
    return detail_expected::and_then(*this, std::forward<F>(f));
  }

  template <typename F>
  auto and_then(F &&f) && {
    return detail_expected::and_then(std::move(*this), std::forward<F>(f));
  }

  template <typename F>
  auto or_else(F &&f) & {
    return detail_expected::or_else(*this, std::forward<F>(f));
  }

  template <typename F>
  auto or_else(F &&f) const & {
    return detail_expected::or_else(*this, std::forward<F>(f));
  }

  template <typename F>
  auto or_else(F &&f) && {
    return detail_expected::or_else(std::move(*this), std::forward<F>(f));
  }
};

// Compares

template <typename T1, typename E1, typename T2, typename E2>
bool operator==(expected<T1, E1> const &lhs, expected<T2, E2> const &rhs) {
  return (lhs.has_value() == rhs.has_value())
      && (lhs.has_value() ? *lhs == *rhs : lhs.error() == rhs.error());
}

template <typename E1, typename E2>
bool operator==(expected<void, E1> const &lhs, expected<void, E2> const &rhs) {
  return (lhs.has_value() == rhs.has_value())
      && (lhs.has_value() || lhs.error() == rhs.error());
}

template <typename T1, typename E1, typename T2, typename E2>
bool operator!=(expected<T1, E1> const &lhs, expected<T2, E2> const &rhs) {
  return !(lhs == rhs);
}

} // namespace ipc
