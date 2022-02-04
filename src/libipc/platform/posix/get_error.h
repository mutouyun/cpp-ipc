#pragma once

#include <errno.h>
#include <string.h>

namespace ipc {
namespace detail {

inline char const *curr_error() noexcept {
    return ::strerror(errno);
}

} // namespace detail
} // namespace ipc
