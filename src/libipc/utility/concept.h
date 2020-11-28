#pragma once

#include <type_traits>  // std::declval

namespace ipc {

/**
 * @remarks
 * <<Concepts emulation>> Concepts TS Improve on C++17
 * @see
 * - http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0726r0.html
 * - https://www.youtube.com/watch?v=TorW5ekkL_w
*/
namespace detail {

template <typename F, typename... Args,
          typename = decltype(std::declval<F&&>()(std::declval<Args&&>()...))>
constexpr bool require_impl(int) { return true; }

template <typename F, typename... Args>
constexpr bool require_impl(...) { return false; }

} // namespace detail

template <typename... Args, typename F>
constexpr bool require(F&&) {
    return detail::require_impl<F&&, Args&&...>(int{});
}

} // namespace ipc

/// concept helpers

#ifdef IPC_CONCEPT_
#   error "IPC_CONCEPT_ has been defined."
#endif

#define IPC_CONCEPT_($$name, $$what)        \
class $$name {                              \
public:                                     \
    constexpr static bool value = $$what;   \
}
