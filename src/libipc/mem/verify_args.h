#pragma once

#include <cstddef>

namespace ipc {
namespace mem {

/**
 * \brief Check that bytes is not 0 and that the alignment is a power of two.
 */
inline constexpr bool verify_args(std::size_t bytes, std::size_t alignment) noexcept {
  return (bytes > 0) && (alignment > 0) && ((alignment & (alignment - 1)) == 0);
}

} // namespace mem
} // namespace ipc
