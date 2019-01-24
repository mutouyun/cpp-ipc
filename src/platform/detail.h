#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <type_traits>

#include "def.h"

#if __cplusplus >= 201703L

namespace std {

// deduction guides for std::unique_ptr
template <typename T, typename D>
unique_ptr(T* p, D&& d) -> unique_ptr<T, std::decay_t<D>>;

} // namespace std

namespace ipc {
namespace detail {

using std::unique_ptr;
using std::unique_lock;
using std::shared_lock;

#else /*__cplusplus < 201703L*/

namespace ipc {
namespace detail {

// deduction guides for std::unique_ptr
template <typename T, typename D>
constexpr auto unique_ptr(T* p, D&& d) {
    return std::unique_ptr<T, std::decay_t<D>> { p, std::forward<D>(d) };
}

// deduction guides for std::unique_lock
template <typename T>
constexpr auto unique_lock(T&& lc) {
    return std::unique_lock<std::decay_t<T>> { std::forward<T>(lc) };
}

// deduction guides for std::shared_lock
template <typename T>
constexpr auto shared_lock(T&& lc) {
    return std::shared_lock<std::decay_t<T>> { std::forward<T>(lc) };
}

#endif/*__cplusplus < 201703L*/

template <typename F, typename D>
constexpr decltype(auto) static_switch(std::size_t /*i*/, std::index_sequence<>, F&& /*f*/, D&& def) {
    return def();
}

template <typename F, typename D, std::size_t N, std::size_t...I>
constexpr decltype(auto) static_switch(std::size_t i, std::index_sequence<N, I...>, F&& f, D&& def) {
    return (i == N) ? f(std::integral_constant<size_t, N>{}) :
                      static_switch(i, std::index_sequence<I...>{}, f, def);
}

template <typename F, std::size_t...I>
#if __cplusplus >= 201703L
constexpr void static_for(std::index_sequence<I...>, F&& f) {
#else /*__cplusplus < 201703L*/
inline void static_for(std::index_sequence<I...>, F&& f) {
#endif/*__cplusplus < 201703L*/
    IPC_UNUSED_ auto expand = { (f(std::integral_constant<size_t, I>{}), 0)... };
}

} // namespace detail
} // namespace ipc
