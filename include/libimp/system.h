/**
 * @file libimp/system.h
 * @author mutouyun (orz@orzz.org)
 * @brief Isolation and encapsulation of system APIs
 * @date 2022-08-07
 */
#pragma once

#include <string>
#include <ostream>  // std::ostream
#include <cstdint>

#include "libimp/def.h"
#include "libimp/export.h"
#include "libimp/result.h"
#include "libimp/fmt_cpo.h"

LIBIMP_NAMESPACE_BEG_
namespace sys {

/**
 * @brief Get/Set the system error code
 */
LIBIMP_EXPORT result_code error_code() noexcept;
LIBIMP_EXPORT void error_code(result_code) noexcept;

/**
 * @brief Gets a text description of the system error
 */
LIBIMP_EXPORT std::string error_str(result_code) noexcept;

/**
 * @brief A text description string with an error number attached
 */
LIBIMP_EXPORT std::string error_msg(result_code) noexcept;

/**
 * @brief The system error object.
 */
class LIBIMP_EXPORT error {
  result_code r_code_;

public:
  explicit error() noexcept;
  explicit error(result_code rc) noexcept;

  result_code code() const noexcept;
  std::uint64_t value() const noexcept;
  explicit operator bool() const noexcept;

  std::string str() const noexcept;

  friend LIBIMP_EXPORT bool operator==(error const &lhs, error const &rhs) noexcept;
  friend LIBIMP_EXPORT bool operator!=(error const &lhs, error const &rhs) noexcept;
};

/**
 * @brief Get system configuration information at run time
 */
enum class info : std::int32_t {
  page_size,
};
LIBIMP_EXPORT result<std::int64_t> conf(info) noexcept;

/**
 * @brief @brief Custom defined fmt_to method for imp::fmt
 */
inline bool tag_invoke(decltype(::LIBIMP::fmt_to), fmt_context &ctx, error r) noexcept {
  return fmt_to(ctx, error_msg(r.code()));
}

} // namespace sys
LIBIMP_NAMESPACE_END_
