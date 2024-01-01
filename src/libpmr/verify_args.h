#pragma once

#include <cstddef>

#include "libpmr/def.h"

LIBPMR_NAMESPACE_BEG_

/**
 * \brief Check that bytes is not 0 and that the alignment is a power of two.
 */
inline bool verify_args(std::size_t bytes, std::size_t alignment) noexcept {
  if (bytes == 0) {
    return false;
  }
  if ((alignment == 0) || (alignment & (alignment - 1)) != 0) {
    return false;
  }
  return true;
}

LIBPMR_NAMESPACE_END_
