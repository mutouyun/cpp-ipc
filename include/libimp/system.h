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

  friend bool operator==(error const &lhs, error const &rhs) noexcept;
  friend bool operator!=(error const &lhs, error const &rhs) noexcept;

  friend std::ostream &operator<<(std::ostream &o, error const &e);
};

/**
 * @brief Get system configuration information at run time
 */
enum class info : std::int32_t {
  page_size,
};
LIBIMP_EXPORT result<std::int64_t> conf(info) noexcept;

} // namespace sys
LIBIMP_NAMESPACE_END_

template <>
struct fmt::formatter<::LIBIMP_::sys::error>
          : formatter<std::string> {
  template <typename FormatContext>
  auto format(::LIBIMP_::sys::error r, FormatContext &ctx) {
    return format_to(ctx.out(), ::LIBIMP_::sys::error_msg(r.code()));
  }
};
