/**
 * \file libipc/uninitialized.h
 * \author mutouyun (orz@orzz.org)
 * \brief Uninitialized memory algorithms.
 */
#pragma once

#include <new>          // placement-new
#include <type_traits>  // std::enable_if_t
#include <utility>      // std::forward
#include <memory>       // std::construct_at, std::destroy_at, std::addressof
#include <cstddef>

#include "libipc/imp/detect_plat.h"
#include "libipc/imp/generic.h"

namespace ipc {

/**
 * \brief Creates an object at a given address, like 'construct_at' in c++20
 * \see https://en.cppreference.com/w/cpp/memory/construct_at
*/

template <typename T, typename... A>
auto construct(void *p, A &&...args)
  -> std::enable_if_t<::std::is_constructible<T, A...>::value, T *> {
#if defined(LIBIPC_CPP_20)
  return std::construct_at(static_cast<T *>(p), std::forward<A>(args)...);
#else
  return ::new (p) T(std::forward<A>(args)...);
#endif
}

template <typename T, typename... A>
auto construct(void *p, A &&...args)
  -> std::enable_if_t<!::std::is_constructible<T, A...>::value, T *> {
  return ::new (p) T{std::forward<A>(args)...};
}

/**
 * \brief Destroys an object at a given address, like 'destroy_at' in c++17
 * \see https://en.cppreference.com/w/cpp/memory/destroy_at
*/

template <typename T>
void *destroy(T *p) noexcept {
  if (p == nullptr) return nullptr;
#if defined(LIBIPC_CPP_17)
  std::destroy_at(p);
#else
  p->~T();
#endif
  return p;
}

template <typename T, std::size_t N>
void *destroy(T (*p)[N]) noexcept {
  if (p == nullptr) return nullptr;
#if defined(LIBIPC_CPP_20)
  std::destroy_at(p);
#elif defined(LIBIPC_CPP_17)
  std::destroy(std::begin(*p), std::end(*p));
#else
  for (auto &elem : *p) destroy(std::addressof(elem));
#endif
  return p;
}

/**
 * \brief Destroys a range of objects.
 * \see https://en.cppreference.com/w/cpp/memory/destroy
*/
template <typename ForwardIt>
void destroy(ForwardIt first, ForwardIt last) noexcept {
#if defined(LIBIPC_CPP_17)
  std::destroy(first, last);
#else
  for (; first != last; ++first) {
    destroy(std::addressof(*first));
  }
#endif
}

/**
 * \brief Destroys a number of objects in a range.
 * \see https://en.cppreference.com/w/cpp/memory/destroy_n
*/
template <typename ForwardIt, typename Size>
ForwardIt destroy_n(ForwardIt first, Size n) noexcept {
#if defined(LIBIPC_CPP_17)
  return std::destroy_n(first, n);
#else
  for (; n > 0; (void) ++first, --n)
    destroy(std::addressof(*first));
  return first;
#endif
}

/**
 * \brief Constructs objects by default-initialization 
 *        in an uninitialized area of memory, defined by a start and a count.
 * \see https://en.cppreference.com/w/cpp/memory/uninitialized_default_construct_n
*/
template <typename ForwardIt, typename Size>
ForwardIt uninitialized_default_construct_n(ForwardIt first, Size n) {
#if defined(LIBIPC_CPP_17)
  return std::uninitialized_default_construct_n(first, n);
#else
  using T = typename std::iterator_traits<ForwardIt>::value_type;
  ForwardIt current = first;
  LIBIPC_TRY {
    for (; n > 0; (void) ++current, --n)
      ::new (horrible_cast<void *>(std::addressof(*current))) T;
    return current;
  } LIBIPC_CATCH(...) {
    destroy(first, current);
    LIBIPC_THROW(, first);
  }
#endif
}

/**
* \brief Moves a number of objects to an uninitialized area of memory.
* \see https://en.cppreference.com/w/cpp/memory/uninitialized_move_n
*/
template <typename InputIt, typename Size, typename NoThrowForwardIt>
auto uninitialized_move_n(InputIt first, Size count, NoThrowForwardIt d_first)
  -> std::pair<InputIt, NoThrowForwardIt> {
#if defined(LIBIPC_CPP_17)
  return std::uninitialized_move_n(first, count, d_first);
#else
  using Value = typename std::iterator_traits<NoThrowForwardIt>::value_type;
  NoThrowForwardIt current = d_first;
  LIBIPC_TRY {
    for (; count > 0; ++first, (void) ++current, --count) {
      ::new (static_cast<void *>(std::addressof(*current))) Value(std::move(*first));
    }
  } LIBIPC_CATCH(...) {
    destroy(d_first, current);
    LIBIPC_THROW(, {first, d_first});
  }
  return {first, current};
#endif
}

} // namespace ipc
