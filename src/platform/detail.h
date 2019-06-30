#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <type_traits>
#include <tuple>
#include <atomic>
#include <algorithm>
#include <utility>
#include <new>

#include "def.h"
#include "export.h"

// pre-defined

#ifdef IPC_UNUSED_
#   error "IPC_UNUSED_ has been defined."
#endif
#ifdef IPC_FALLTHROUGH_
#   error "IPC_FALLTHROUGH_ has been defined."
#endif
#ifdef IPC_STBIND_
#   error "IPC_STBIND_ has been defined."
#endif

#if __cplusplus >= 201703L

#define IPC_UNUSED_      [[maybe_unused]]
#define IPC_FALLTHROUGH_ [[fallthrough]]
#define IPC_STBIND_(A, B, ...) auto [A, B] = __VA_ARGS__

#else /*__cplusplus < 201703L*/

#if defined(_MSC_VER)
#   define IPC_UNUSED_ __pragma(warning(suppress: 4100 4101 4189))
#elif defined(__GNUC__)
#   define IPC_UNUSED_ __attribute__((__unused__))
#else
#   define IPC_UNUSED_
#endif

#define IPC_FALLTHROUGH_

#define IPC_STBIND_(A, B, ...)       \
    auto tp___ = __VA_ARGS__         \
    auto A     = std::get<0>(tp___); \
    auto B     = std::get<1>(tp___)

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
using std::max;
using std::min;

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

template <typename T>
constexpr const T& (max)(const T& a, const T& b) {
    return (a < b) ? b : a;
}

template <typename T>
constexpr const T& (min)(const T& a, const T& b) {
    return (b < a) ? b : a;
}

#endif/*__cplusplus < 201703L*/

template <typename F, typename D>
constexpr decltype(auto) static_switch(std::size_t /*i*/, std::index_sequence<>, F&& /*f*/, D&& def) {
    return std::forward<D>(def)();
}

template <typename F, typename D, std::size_t N, std::size_t...I>
constexpr decltype(auto) static_switch(std::size_t i, std::index_sequence<N, I...>, F&& f, D&& def) {
    return (i == N) ? std::forward<F>(f)(std::integral_constant<size_t, N>{}) :
                      static_switch(i, std::index_sequence<I...>{}, std::forward<F>(f), std::forward<D>(def));
}

template <std::size_t N, typename F, typename D>
constexpr decltype(auto) static_switch(std::size_t i, F&& f, D&& def) {
    return static_switch(i, std::make_index_sequence<N>{}, std::forward<F>(f), std::forward<D>(def));
}

template <typename F, std::size_t...I>
#if __cplusplus >= 201703L
constexpr void static_for(std::index_sequence<I...>, F&& f) {
#else /*__cplusplus < 201703L*/
inline void static_for(std::index_sequence<I...>, F&& f) {
#endif/*__cplusplus < 201703L*/
    IPC_UNUSED_ auto expand = { (std::forward<F>(f)(std::integral_constant<size_t, I>{}), 0)... };
}

template <std::size_t N, typename F>
#if __cplusplus >= 201703L
constexpr void static_for(F&& f) {
#else /*__cplusplus < 201703L*/
inline void static_for(F&& f) {
#endif/*__cplusplus < 201703L*/
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

} // namespace detail
} // namespace ipc
