#ifndef LIBIPC_SRC_PLATFORM_DETAIL_H_
#define LIBIPC_SRC_PLATFORM_DETAIL_H_

// detect platform

#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || \
    defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || \
    defined(WINCE) || defined(_WIN32_WCE)
#   define IPC_OS_WINDOWS_
#elif defined(__linux__) || defined(__linux)
#   define IPC_OS_LINUX_
#elif defined(__QNX__)
#   define IPC_OS_QNX_
#elif defined(__APPLE__)
#elif defined(__ANDROID__)
// TBD
#endif

#if defined(__cplusplus)

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <type_traits>
#include <tuple>
#include <algorithm>

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
#ifdef IPC_CONSTEXPR_
#   error "IPC_CONSTEXPR_ has been defined."
#endif

#if __cplusplus >= 201703L

#define IPC_UNUSED_      [[maybe_unused]]
#define IPC_FALLTHROUGH_ [[fallthrough]]
#define IPC_STBIND_(A, B, ...) auto [A, B] = __VA_ARGS__
#define IPC_CONSTEXPR_   constexpr

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

#define IPC_CONSTEXPR_ inline

#endif/*__cplusplus < 201703L*/

#if __cplusplus >= 201703L

namespace std {

// deduction guides for std::unique_ptr
template <typename T>
unique_ptr(T* p) -> unique_ptr<T>;
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
template <typename T>
constexpr auto unique_ptr(T* p) noexcept {
    return std::unique_ptr<T> { p };
}

template <typename T, typename D>
constexpr auto unique_ptr(T* p, D&& d) noexcept {
    return std::unique_ptr<T, std::decay_t<D>> { p, std::forward<D>(d) };
}

// deduction guides for std::unique_lock
template <typename T>
constexpr auto unique_lock(T&& lc) noexcept {
    return std::unique_lock<std::decay_t<T>> { std::forward<T>(lc) };
}

// deduction guides for std::shared_lock
template <typename T>
constexpr auto shared_lock(T&& lc) noexcept {
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

} // namespace detail
} // namespace ipc

#endif // defined(__cplusplus)
#endif // LIBIPC_SRC_PLATFORM_DETAIL_H_
