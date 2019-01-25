#pragma once

#include <cstdio>
#include <utility>

namespace ipc {

template <typename... P>
void log(char const * fmt, P&&... params) {
    std::fprintf(stderr, fmt, std::forward<P>(params)...);
}

} // namespace ipc
