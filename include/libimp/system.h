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
#include "libimp/error.h"

LIBIMP_NAMESPACE_BEG_
namespace sys {

/**
 * @brief Get/Set the system error number
 */
LIBIMP_EXPORT result_code error_no() noexcept;
LIBIMP_EXPORT void error_no(result_code) noexcept;

/**
 * @brief Gets a text description of the system error
 */
LIBIMP_EXPORT std::string error_str(result_code) noexcept;

/**
 * @brief Identifies the operating system error category.
 * @see https://en.cppreference.com/w/cpp/error/system_category
 */
LIBIMP_EXPORT error_category const &category() noexcept;

/**
 * @brief A platform-dependent error code.
 * @see https://en.cppreference.com/w/cpp/error/error_code
 */
LIBIMP_EXPORT error_code error() noexcept;

/**
 * @brief Get system configuration information at run time
 */
enum class info : std::int32_t {
  page_size,
};
LIBIMP_EXPORT result<std::int64_t> conf(info) noexcept;

} // namespace sys
LIBIMP_NAMESPACE_END_
