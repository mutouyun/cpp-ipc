#pragma once

#include <cstddef>

#include "memory/alloc.h"
#include "platform/detail.h"

namespace ipc {
namespace mem {
namespace detail {

enum : std::size_t {
    base_size = sizeof(void*)
};

template <std::size_t Size, template <std::size_t> class FixedAlloc>
struct fixed {
    static auto& pool() {
        static FixedAlloc<Size> pool;
        return pool;
    }
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

template <template <std::size_t> class Fixed, typename F>
decltype(auto) choose(std::size_t size, F&& f) {
    return ipc::detail::static_switch(classify(size), std::make_index_sequence<32> {
    }, [&f](auto index) {
        return f(Fixed<(decltype(index)::value + 1) * base_size>::pool());
    }, [&f] {
        return f(static_alloc{});
    });
}

template <template <std::size_t> class Fixed>
class pool_alloc {
public:
    static void clear() {
        ipc::detail::static_for(std::make_index_sequence<32> {}, [](auto index) {
            Fixed<(decltype(index)::value + 1) * base_size>::pool().clear();
        });
    }

    static void* alloc(std::size_t size) {
        return choose<Fixed>(size, [size](auto&& fp) { return fp.alloc(size); });
    }

    static void free(void* p, std::size_t size) {
        choose<Fixed>(size, [p](auto&& fp) { fp.free(p); });
    }
};

} // namespace detail
} // namespace mem
} // namespace ipc
