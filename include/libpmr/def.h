/**
 * \file libpmr/def.h
 * \author mutouyun (orz@orzz.org)
 * \brief Define the trivial configuration information for memory resources.
 * \date 2022-11-13
 */
#pragma once

#include <cstddef>

#include "libimp/aligned.h"

#define LIBPMR                pmr
#define LIBPMR_NAMESPACE_BEG_ namespace LIBPMR {
#define LIBPMR_NAMESPACE_END_ }

LIBPMR_NAMESPACE_BEG_

/// \brief Constants.

enum : std::size_t {
  central_cache_default_size = 1024 * 1024, ///< 1MB
  regular_head_size          = ::LIBIMP::round_up(sizeof(std::size_t), alignof(std::max_align_t)),
};

LIBPMR_NAMESPACE_END_
