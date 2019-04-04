#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
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
    invalid_value  = (std::numeric_limits<std::size_t>::max)(),
    data_length    = 64,
    default_timeut = 100 // ms
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

} // namespace ipc
