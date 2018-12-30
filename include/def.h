#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <new>
#include <utility>

namespace ipc {

// types

using byte_t = std::uint8_t;

template <std::size_t N>
struct uint;

template <> struct uint<8 > { using type = std::uint8_t ; };
template <> struct uint<16> { using type = std::uint16_t; };
template <> struct uint<32> { using type = std::uint32_t; };
template <> struct uint<64> { using type = std::uint64_t; };

template <std::size_t N>
using uint_t = typename uint<N>::type;

// constants

enum : std::size_t {
    invalid_value = (std::numeric_limits<std::size_t>::max)(),
    data_length   = 16
};

// concept helpers

template <bool Cond, typename R>
using Requires = std::enable_if_t<Cond, R>;

// pimpl small object optimization helpers

template <typename T, typename R = T*>
using IsImplComfortable = Requires<(sizeof(T) <= sizeof(T*)), R>;

template <typename T, typename R = T*>
using IsImplUncomfortable = Requires<(sizeof(T) > sizeof(T*)), R>;

template <typename T, typename... P>
constexpr auto make_impl(P&&... params) -> IsImplComfortable<T> {
    T* buf {};
    ::new (&buf) T { std::forward<P>(params)... };
    return buf;
}

template <typename T>
constexpr auto impl(T* const (& p)) -> IsImplComfortable<T> {
    return reinterpret_cast<T*>(&const_cast<char &>(reinterpret_cast<char const &>(p)));
}

template <typename T>
constexpr auto clear_impl(T* p) -> IsImplComfortable<T, void> {
    if (p != nullptr) impl(p)->~T();
}

template <typename T, typename... P>
constexpr auto make_impl(P&&... params) -> IsImplUncomfortable<T> {
    return new T { std::forward<P>(params)... };
}

template <typename T>
constexpr auto clear_impl(T* p) -> IsImplUncomfortable<T, void> {
    delete p;
}

template <typename T>
constexpr auto impl(T* const (& p)) -> IsImplUncomfortable<T> {
    return p;
}

template <typename T>
struct pimpl {
    template <typename... P>
    constexpr static T* make(P&&... params) {
        return make_impl<T>(std::forward<P>(params)...);
    }

    constexpr void clear() {
        clear_impl(static_cast<T*>(this));
    }
};

} // namespace ipc
