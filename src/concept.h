#pragma once

#include <type_traits>

namespace ipc {

// concept helpers

template <bool Cond, typename R>
using Requires = std::enable_if_t<Cond, R>;

} // namespace ipc
