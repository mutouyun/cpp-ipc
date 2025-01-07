/**
 * \file libipc/byte.h
 * \author mutouyun (orz@orzz.org)
 * \brief Define the byte type.
 */
#pragma once

#include <type_traits>
#include <cstdint>
#include <cstddef>  // std::byte (since C++17)

#include "libipc/imp/detect_plat.h"
#include "libipc/imp/span.h"
#include "libipc/imp/fmt.h"

#if defined(LIBIPC_CPP_17) && defined(__cpp_lib_byte)
#define LIBIPC_CPP_LIB_BYTE_
#endif // __cpp_lib_byte

namespace ipc {

class byte;

namespace detail_byte {

template <typename T>
using is_integral = 
  typename std::enable_if<std::is_integral<T>::value>::type;

template <typename T>
using is_not_byte = 
  typename std::enable_if<!std::is_same<
  typename std::remove_cv<T>::type, byte>::value>::type;

} // namespace detail_byte

/**
 * \brief A distinct type that implements the concept of byte as specified in the C++ language definition.
 * \see https://en.cppreference.com/w/cpp/types/byte
 */
class byte {
  std::uint8_t bits_;

public:
  byte() noexcept = default;

  template <typename T, typename = detail_byte::is_integral<T>>
  constexpr byte(T v) noexcept
    : bits_(static_cast<std::uint8_t>(v)) {}

#ifdef LIBIPC_CPP_LIB_BYTE_
  constexpr byte(std::byte b) noexcept
    : byte(std::to_integer<std::uint8_t>(b)) {}
#endif // LIBIPC_CPP_LIB_BYTE_

  template <typename T, typename = detail_byte::is_integral<T>>
  constexpr operator T() const noexcept {
    return static_cast<T>(bits_);
  }

#ifdef LIBIPC_CPP_LIB_BYTE_
  constexpr operator std::byte() const noexcept {
    /// \brief C++17 relaxed enum class initialization rules.
    /// \see https://en.cppreference.com/w/cpp/language/enum#enum_relaxed_init_cpp17
    return std::byte{bits_};
  }
#endif // LIBIPC_CPP_LIB_BYTE_

  friend bool operator==(byte const &lhs, byte const &rhs) noexcept {
    return lhs.bits_ == rhs.bits_;
  }

  friend bool operator!=(byte const &lhs, byte const &rhs) noexcept {
    return !(lhs == rhs);
  }
};

/**
 * \brief Non-member functions.
 */

template <typename T, typename = detail_byte::is_integral<T>>
constexpr T to_integer(byte b) noexcept {
  return T(b);
}

/// \brief std::operator<<, operator>>

template <typename T, typename = detail_byte::is_integral<T>>
constexpr byte operator<<(byte b, T shift) noexcept {
  return byte(to_integer<unsigned>(b) << shift);
}

template <typename T, typename = detail_byte::is_integral<T>>
constexpr byte operator>>(byte b, T shift) noexcept {
  return byte(to_integer<unsigned>(b) >> shift);
}

/// \brief std::operator<<=, operator>>=

template <typename T, typename = detail_byte::is_integral<T>>
constexpr byte &operator<<=(byte &b, T shift) noexcept {
  return b = b << shift;
}

template <typename T, typename = detail_byte::is_integral<T>>
constexpr byte &operator>>=(byte &b, T shift) noexcept {
  return b = b >> shift;
}

/// \brief std::operator|, operator&, operator^, operator~

constexpr byte operator|(byte l, byte r) noexcept { return byte(to_integer<unsigned>(l) | to_integer<unsigned>(r)); }
constexpr byte operator&(byte l, byte r) noexcept { return byte(to_integer<unsigned>(l) & to_integer<unsigned>(r)); }
constexpr byte operator^(byte l, byte r) noexcept { return byte(to_integer<unsigned>(l) ^ to_integer<unsigned>(r)); }
constexpr byte operator~(byte b)         noexcept { return byte(~to_integer<unsigned>(b)); }

/// \brief std::operator|=, operator&=, operator^=

constexpr byte &operator|=(byte &l, byte r) noexcept { return l = l | r; }
constexpr byte &operator&=(byte &l, byte r) noexcept { return l = l & r; }
constexpr byte &operator^=(byte &l, byte r) noexcept { return l = l ^ r; }

/// \brief Cast pointer to byte*.

template <typename T, typename = detail_byte::is_not_byte<T>>
byte *byte_cast(T *p) noexcept {
  return reinterpret_cast<byte *>(p);
}

template <typename T, typename = detail_byte::is_not_byte<T>>
byte const *byte_cast(T const *p) noexcept {
  return reinterpret_cast<byte const *>(p);
}

/// \brief Cast byte* to a pointer of another type.

template <typename T, typename = detail_byte::is_not_byte<T>>
T *byte_cast(byte *p) noexcept {
  if (reinterpret_cast<std::size_t>(p) % alignof(T) != 0) {
    return nullptr;
  }
  return reinterpret_cast<T *>(p);
}

template <typename T, typename U = typename std::add_const<T>::type, 
          typename = detail_byte::is_not_byte<T>>
U *byte_cast(byte const *p) noexcept {
  if (reinterpret_cast<std::size_t>(p) % alignof(T) != 0) {
    return nullptr;
  }
  return reinterpret_cast<U *>(p);
}

/// \brief Converts a span into a view of its underlying bytes.
/// \see https://en.cppreference.com/w/cpp/container/span/as_bytes

template <typename T, 
          typename Byte = typename std::conditional<std::is_const<T>::value, byte const, byte>::type>
auto as_bytes(span<T> s) noexcept -> span<Byte> {
  return {byte_cast(s.data()), s.size_bytes()};
}

/// \brief Custom defined fmt_to method for imp::fmt
namespace detail_tag_invoke {

inline bool tag_invoke(decltype(ipc::fmt_to), fmt_context &ctx, ipc::byte b) {
  return ipc::to_string(ctx, static_cast<std::uint8_t>(b), "02x");
}

template <typename T, 
          typename = std::enable_if_t<std::is_same<std::decay_t<T>, ipc::byte>::value>>
bool tag_invoke(decltype(ipc::fmt_to), fmt_context &ctx, fmt_ref<T> arg) noexcept {
  return ipc::to_string(ctx, static_cast<std::uint8_t>(arg.param), arg.fstr);
}

} // namespace detail_tag_invoke
} // namespace ipc
