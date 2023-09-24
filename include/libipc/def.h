#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>       // std::numeric_limits
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

enum : std::uint32_t {
    invalid_value   = (std::numeric_limits<std::uint32_t>::max)(),
    default_timeout = 100, // ms
};

enum : std::size_t {
    data_length     = 64,
    large_msg_limit = data_length,
    large_msg_align = 1024,
    large_msg_cache = 32,
};

enum class relat { // multiplicity of the relationship
    single,
    multi
};

enum class trans { // transmission
    unicast,
    broadcast
};

// producer-consumer policy flag

template <relat Rp, relat Rc, trans Ts>
struct wr {};

template <typename WR>
struct relat_trait;

template <relat Rp, relat Rc, trans Ts>
struct relat_trait<wr<Rp, Rc, Ts>> {
    constexpr static bool is_multi_producer = (Rp == relat::multi);
    constexpr static bool is_multi_consumer = (Rc == relat::multi);
    constexpr static bool is_broadcast      = (Ts == trans::broadcast);
};

template <template <typename> class Policy, typename Flag>
struct relat_trait<Policy<Flag>> : relat_trait<Flag> {};

// the prefix tag of a channel
struct prefix {
    char const *str;
};

} // namespace ipc
