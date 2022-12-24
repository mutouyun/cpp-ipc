/**
 * \file libimp/error.h
 * \author mutouyun (orz@orzz.org)
 * \brief A platform-dependent error code.
 * \date 2022-12-18
 */
#pragma once

#include <string>
#include <cstdint>

#include "libimp/def.h"
#include "libimp/export.h"
#include "libimp/fmt_cpo.h"

LIBIMP_NAMESPACE_BEG_

using error_code_t = std::uint64_t;

/**
 * \brief Serves as the base class for specific error category types.
 * \see https://en.cppreference.com/w/cpp/error/error_category
 */
class LIBIMP_EXPORT error_category {
public:
  error_category(error_category const &) = delete;
  error_category &operator=(error_category const &) = delete;

  constexpr error_category() noexcept = default;
  virtual ~error_category() noexcept = default;

  /// \brief observer
  virtual std::string name() const = 0;
  virtual std::string message(error_code_t const &r) const = 0;

  /// \brief comparison function
  bool operator==(error_category const &rhs) const noexcept;
};

/**
 * \brief Identifies the generic error category.
 * \see https://en.cppreference.com/w/cpp/error/generic_category
 */
LIBIMP_EXPORT error_category const &generic_category() noexcept;

/**
 * \brief The error code object.
 * \see https://en.cppreference.com/w/cpp/error/error_code
 */
class LIBIMP_EXPORT error_code {
  error_code_t e_code_;
  error_category const *ec_;

public:
  /// \brief constructors
  error_code() noexcept;
  error_code(error_code_t const &r, error_category const &ec) noexcept;

  /// \brief observers
  error_code_t code() const noexcept;
  error_category const &category() const noexcept;
  std::string message() const;
  explicit operator bool() const noexcept;

  /// \brief comparison functions
  friend LIBIMP_EXPORT bool operator==(error_code const &lhs, error_code const &rhs) noexcept;
  friend LIBIMP_EXPORT bool operator!=(error_code const &lhs, error_code const &rhs) noexcept;
};

/**
 * \brief @brief Custom defined fmt_to method for imp::fmt
 */
namespace detail {

inline bool tag_invoke(decltype(::LIBIMP::fmt_to), fmt_context &ctx, error_code r) noexcept {
  return fmt_to(ctx, r.message());
}

} // namespace detail
LIBIMP_NAMESPACE_END_
