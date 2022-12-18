/**
 * @file libimp/error.h
 * @author mutouyun (orz@orzz.org)
 * @brief A platform-dependent error code.
 * @date 2022-12-18
 */
#pragma once

#include <string>

#include "libimp/def.h"
#include "libimp/export.h"
#include "libimp/result.h"
#include "libimp/fmt_cpo.h"

LIBIMP_NAMESPACE_BEG_

/**
 * @brief Serves as the base class for specific error category types.
 * @see https://en.cppreference.com/w/cpp/error/error_category
 */
class LIBIMP_EXPORT error_category {
public:
  error_category(error_category const &) = delete;
  error_category &operator=(error_category const &) = delete;

  constexpr error_category() noexcept = default;
  virtual ~error_category() noexcept = default;

  /// @brief observer
  virtual std::string message(result_code r) const = 0;

  /// @brief comparison function
  bool operator==(error_category const &rhs) const noexcept;
};

/**
 * @brief Identifies the generic error category.
 * @see https://en.cppreference.com/w/cpp/error/generic_category
 */
LIBIMP_EXPORT error_category const &generic_category() noexcept;

/**
 * @brief The error code object.
 * @see https://en.cppreference.com/w/cpp/error/error_code
 */
class LIBIMP_EXPORT error_code {
  result_code r_code_;
  error_category const *ec_;

public:
  /// @brief constructors
  error_code() noexcept;
  error_code(result_code r, error_category const &ec) noexcept;

  /// @brief observers
  result_code code() const noexcept;
  result_type value() const noexcept;
  error_category const &category() const noexcept;
  std::string message() const;
  explicit operator bool() const noexcept;

  /// @brief comparison functions
  friend LIBIMP_EXPORT bool operator==(error_code const &lhs, error_code const &rhs) noexcept;
  friend LIBIMP_EXPORT bool operator!=(error_code const &lhs, error_code const &rhs) noexcept;
};

LIBIMP_NAMESPACE_END_
