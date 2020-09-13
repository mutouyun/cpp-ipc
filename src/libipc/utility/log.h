#pragma once

#include <cstdio>
#include <utility>

namespace ipc {
namespace detail {

template <typename O>
void print(O out, char const * fmt) {
    std::fprintf(out, "%s", fmt);
}

template <typename O, typename P1, typename... P>
void print(O out, char const * fmt, P1&& p1, P&&... params) {
    std::fprintf(out, fmt, std::forward<P1>(p1), std::forward<P>(params)...);
}

} // namespace detail

inline void log(char const * fmt) {
    ipc::detail::print(stdout, fmt);
}

template <typename P1, typename... P>
void log(char const * fmt, P1&& p1, P&&... params) {
    ipc::detail::print(stdout, fmt, std::forward<P1>(p1), std::forward<P>(params)...);
}

inline void error(char const * fmt) {
    ipc::detail::print(stderr, fmt);
}

template <typename P1, typename... P>
void error(char const * fmt, P1&& p1, P&&... params) {
    ipc::detail::print(stderr, fmt, std::forward<P1>(p1), std::forward<P>(params)...);
}

} // namespace ipc
