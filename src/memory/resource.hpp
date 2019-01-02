#pragma once

#include <type_traits>
#include <limits>
#include <utility>
#include <functional>
#include <unordered_map>
#include <vector>

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
constexpr void static_for(std::index_sequence<I...>, F&& f) {
    [[maybe_unused]] auto expand = { (f(std::integral_constant<size_t, I>{}), 0)... };
}

template <std::size_t Size>
auto& fixed() {
    static synchronized<fixed_pool<Size>> pool;
    return pool;
}

enum : std::size_t {
    base_size = sizeof(void*)
};

using fixed_sequence_t = std::index_sequence<
    base_size     , base_size * 2 ,
    base_size * 3 , base_size * 4 ,
    base_size * 5 , base_size * 6 ,
    base_size * 7 , base_size * 8 ,
    base_size * 9 , base_size * 10,
    base_size * 11, base_size * 12,
    base_size * 13, base_size * 14,
    base_size * 15, base_size * 16
>;

template <typename F>
decltype(auto) choose(std::size_t size, F&& f) {
    size = ((size - 1) & (~(base_size - 1))) + base_size;
    return detail::static_switch(size, fixed_sequence_t {
    }, [&f](auto index) {
        return f(fixed<decltype(index)::value>());
    }, [&f] {
        return f(static_alloc{});
    });
}

class pool_alloc {
public:
    static void clear() {
        static_for(fixed_sequence_t {}, [](auto index) {
            fixed<decltype(index)::value>().clear();
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
