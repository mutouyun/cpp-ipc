#pragma once

#include <type_traits>
#include <limits>
#include <utility>
#include <functional>
#include <unordered_map>
#include <vector>

#include "def.h"

#include "memory/alloc.hpp"
#include "memory/wrapper.hpp"

namespace ipc {
namespace mem {

namespace detail {

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

template <std::size_t Size>
auto& fixed() {
    static synchronized<fixed_pool<Size>> pool;
    return pool;
}

enum : std::size_t {
    base_size = sizeof(void*)
};

#if __cplusplus >= 201703L
constexpr std::size_t classify(std::size_t size) {
    constexpr std::size_t mapping[] = {
#else /*__cplusplus < 201703L*/
inline std::size_t classify(std::size_t size) {
    static const std::size_t mapping[] = {
#endif/*__cplusplus < 201703L*/
        /* 1 */
        0 , 1 , 2 , 3 ,
        /* 2 */
        5 , 5 , 7 , 7 ,
        9 , 9 , 11, 11,
        13, 13, 15, 15,
        /* 4 */
        19, 19, 19, 19,
        23, 23, 23, 23,
        27, 27, 27, 27,
        31, 31, 31, 31
    };
    size = (size - 1) / base_size;
#if __cplusplus >= 201703L
    return (size < std::size(mapping)) ? mapping[size] : 32;
#else /*__cplusplus < 201703L*/
    return (size < (sizeof(mapping) / sizeof(mapping[0]))) ? mapping[size] : 32;
#endif/*__cplusplus < 201703L*/
}

template <typename F>
decltype(auto) choose(std::size_t size, F&& f) {
    return detail::static_switch(classify(size), std::make_index_sequence<32> {
    }, [&f](auto index) {
        return f(fixed<(decltype(index)::value + 1) * base_size>());
    }, [&f] {
        return f(static_alloc{});
    });
}

class pool_alloc {
public:
    static void clear() {
        static_for(std::make_index_sequence<32> {}, [](auto index) {
            fixed<(decltype(index)::value + 1) * base_size>().clear();
        });
    }

    static void* alloc(std::size_t size) {
        return choose(size, [size](auto&& fp) { return fp.alloc(size); });
    }

    static void free(void* p, std::size_t size) {
        choose(size, [p](auto&& fp) { fp.free(p); });
    }
};

} // namespace detail

template <typename T>
using allocator = allocator_wrapper<T, detail::pool_alloc>;

template <typename Key, typename T>
using unordered_map = std::unordered_map<
    Key, T, std::hash<Key>, std::equal_to<Key>, allocator<std::pair<const Key, T>>
>;

template <typename T>
using vector = std::vector<T, allocator<T>>;

} // namespace mem
} // namespace ipc
