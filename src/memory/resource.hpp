#pragma once

#include <type_traits>
#include <initializer_list>
#include <limits>
#include <utility>
#include <functional>
#include <unordered_map>

#include "tls_pointer.h"

#include "memory/alloc.hpp"
#include "memory/wrapper.hpp"

namespace ipc {
namespace memory {

namespace detail {

template <typename Comp, typename F, std::size_t...I>
void switch_constexpr(std::size_t i, std::index_sequence<I...>, F&& f) {
    [[maybe_unused]] std::initializer_list<int> expand {
        (Comp{}(i, I) && (f(std::integral_constant<size_t, I>{}), 0))...
    };
}

} // namespace detail

class pool_alloc {
private:
    template <std::size_t Size>
    static auto& fixed() {
        static tls::pointer<fixed_pool<Size>> fp;
        return *fp.create();
    }

    template <typename F>
    static void choose(std::size_t size, F&& f) {
        enum : std::size_t { base_size = sizeof(void*) };
        detail::switch_constexpr<std::less_equal<std::size_t>>(size, std::index_sequence<
            base_size     , base_size * 2 ,
            base_size * 4 , base_size * 6 ,
            base_size * 8 , base_size * 12,
            base_size * 16, base_size * 20,
            base_size * 24, base_size * 28,
            base_size * 32, base_size * 40,
            base_size * 48, base_size * 56,
            base_size * 64
        >{}, [&f](auto index) { f(fixed<decltype(index)::value>()); });
    }

public:
    pool_alloc() = default;
    pool_alloc(const pool_alloc&) = default;
    pool_alloc& operator=(const pool_alloc&) = default;
    pool_alloc(pool_alloc&&) = default;
    pool_alloc& operator=(pool_alloc&&) = default;

    static constexpr std::size_t remain() {
        return (std::numeric_limits<std::size_t>::max)();
    }

    static constexpr void clear() {}

    static void* alloc(std::size_t size) {
        void* p;
        choose(size, [&p](auto& fp) { p = fp.alloc(); });
        return p;
    }

    static void free(void* p, std::size_t size) {
        choose(size, [p](auto& fp) { fp.free(p); });
    }
};

template <typename T>
using allocator = allocator_wrapper<T, pool_alloc>;

template<typename Key, typename T>
using unordered_map = std::unordered_map<
    Key, T//, std::hash<Key>, std::equal_to<Key>, allocator<std::pair<const Key, T>>
>;

} // namespace memory
} // namespace ipc
