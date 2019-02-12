#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <type_traits>
#include <tuple>
#include <atomic>

#include "def.h"
#include "export.h"

// pre-defined

#ifdef IPC_UNUSED_
#   error "IPC_UNUSED_ has been defined."
#endif

#if __cplusplus >= 201703L
#   define IPC_UNUSED_ [[maybe_unused]]
#else /*__cplusplus < 201703L*/
#if defined(_MSC_VER)
#   define IPC_UNUSED_ __pragma(warning(suppress: 4100 4101 4189))
#elif defined(__GNUC__)
#   define IPC_UNUSED_ __attribute__((__unused__))
#else
#   define IPC_UNUSED_
#endif
#endif/*__cplusplus < 201703L*/

#ifdef IPC_STBIND_
#   error "IPC_STBIND_ has been defined."
#endif

#if __cplusplus >= 201703L
#   define IPC_STBIND_(A, B, ...) auto [A, B] = __VA_ARGS__
#else /*__cplusplus < 201703L*/
#   define IPC_STBIND_(A, B, ...) \
    auto tp = __VA_ARGS__         \
    auto A  = std::get<0>(tp);    \
    auto B  = std::get<1>(tp)
#endif/*__cplusplus < 201703L*/

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

IPC_EXPORT std::size_t calc_unique_id();

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
