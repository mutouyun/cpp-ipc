/**
 * @file libconcur/concurrent.h
 * @author mutouyun (orz@orzz.org)
 * @brief Define different policies for concurrent queue.
 * @date 2022-11-07
 */
#pragma once

#include <cstdint>
#include <limits>

#include "libconcur/def.h"

LIBCONCUR_NAMESPACE_BEG_

/// @brief The queue index type.
using index_t = std::uint32_t;

namespace state {

/// @brief The state flag type for the queue element.
using flag_t = std::uint64_t;

enum : flag_t {
  /// @brief The invalid state value.
  invalid_value = (std::numeric_limits<flag_t>::max)(),
};

} // namespace state

LIBCONCUR_NAMESPACE_END_
