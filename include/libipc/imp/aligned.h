/**
 * \file libipc/aligned.h
 * \author mutouyun (orz@orzz.org)
 * \brief Defines the type suitable for use as uninitialized storage for types of given type.
 */
#pragma once

#include <array>
#include <cstddef>

#include "libipc/imp/byte.h"

namespace ipc {

/**
 * \brief The type suitable for use as uninitialized storage for types of given type.
 *        std::aligned_storage is deprecated in C++23, so we define our own.
 * \tparam T The type to be aligned.
 * \tparam AlignT The alignment of the type.
 * \see https://en.cppreference.com/w/cpp/types/aligned_storage
 *      https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p1413r3.pdf
 */
template <typename T, std::size_t AlignT = alignof(T)>
class aligned {
  alignas(AlignT) std::array<ipc::byte, sizeof(T)> storage_;

public:
  /**
   * \brief Returns a pointer to the aligned storage.
   * \return A pointer to the aligned storage.
   */
  T *ptr() noexcept {
    return reinterpret_cast<T *>(storage_.data());
  }

  /**
   * \brief Returns a pointer to the aligned storage.
   * \return A pointer to the aligned storage.
   */
  T const *ptr() const noexcept {
    return reinterpret_cast<const T *>(storage_.data());
  }

  /**
   * \brief Returns a reference to the aligned storage.
   * \return A reference to the aligned storage.
   */
  T &ref() noexcept {
    return *ptr();
  }

  /**
   * \brief Returns a reference to the aligned storage.
   * \return A reference to the aligned storage.
   */
  T const &ref() const noexcept {
    return *ptr();
  }
};

/**
 * \brief Rounds up the given value to the given alignment.
 * \tparam T The type of the value.
 * \param value The value to be rounded up.
 * \param alignment The alignment to be rounded up to.
 * \return The rounded up value.
 * \see https://stackoverflow.com/questions/3407012/c-rounding-up-to-the-nearest-multiple-of-a-number
*/
template <typename T>
constexpr T round_up(T value, T alignment) noexcept {
  return (value + alignment - 1) & ~(alignment - 1);
}

} // namespace ipc
