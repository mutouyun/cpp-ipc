#pragma once

#include <utility>      // std::forward, std::integer_sequence
#include <cstddef>      // std::size_t
#include <new>          // std::hardware_destructive_interference_size
#include <type_traits>  // std::is_trivially_copyable

#include "libipc/platform/detail.h"

namespace ipc {

template <typename F, typename D>
constexpr decltype(auto) static_switch(std::size_t /*i*/, std::index_sequence<>, F&& /*f*/, D&& def) {
    return std::forward<D>(def)();
}

template <typename F, typename D, std::size_t N, std::size_t...I>
constexpr decltype(auto) static_switch(std::size_t i, std::index_sequence<N, I...>, F&& f, D&& def) {
    return (i == N) ? std::forward<F>(f)(std::integral_constant<std::size_t, N>{}) :
                      static_switch(i, std::index_sequence<I...>{}, std::forward<F>(f), std::forward<D>(def));
}

template <std::size_t N, typename F, typename D>
constexpr decltype(auto) static_switch(std::size_t i, F&& f, D&& def) {
    return static_switch(i, std::make_index_sequence<N>{}, std::forward<F>(f), std::forward<D>(def));
}

template <typename F, std::size_t...I>
IPC_CONSTEXPR_ void static_for(std::index_sequence<I...>, F&& f) {
    IPC_UNUSED_ auto expand = { (std::forward<F>(f)(std::integral_constant<std::size_t, I>{}), 0)... };
}

template <std::size_t N, typename F>
IPC_CONSTEXPR_ void static_for(F&& f) {
    static_for(std::make_index_sequence<N>{}, std::forward<F>(f));
}

// Minimum offset between two objects to avoid false sharing.
enum {
// #if __cplusplus >= 201703L
//     cache_line_size = std::hardware_destructive_interference_size
// #else /*__cplusplus < 201703L*/
    cache_line_size = 64
// #endif/*__cplusplus < 201703L*/
};

template <typename T, typename U>
auto horrible_cast(U rhs) noexcept
    -> typename std::enable_if<std::is_trivially_copyable<T>::value
                            && std::is_trivially_copyable<U>::value, T>::type {
    union {
        T t;
        U u;
    } r = {};
    r.u = rhs;
    return r.t;
}

IPC_CONSTEXPR_ std::size_t make_align(std::size_t align, std::size_t size) {
    // align must be 2^n
    return (size + align - 1) & ~(align - 1);
}

} // namespace ipc
