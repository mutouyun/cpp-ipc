#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace ipc {

// types

using byte_t = std::uint8_t;

// constants

enum : std::size_t {
    error_count = std::numeric_limits<std::size_t>::max(),
    data_length = 16
};

} // namespace ipc
