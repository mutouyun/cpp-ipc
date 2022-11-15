/**
 * @file libpmr/allocator.h
 * @author mutouyun (orz@orzz.org)
 * @brief A generic polymorphic memory allocator.
 * @date 2022-11-13
 */
#pragma once

#include <type_traits>

#include "libimp/export.h"

#include "libpmr/def.h"
#include "libpmr/memory_resource.h"

LIBPMR_NAMESPACE_BEG_
namespace detail {

/// @brief Helper trait for allocator.

template <typename T, typename = void>
struct has_allocate : std::false_type {};

template <typename T>
struct has_allocate<T, 
  typename std::enable_if<std::is_convertible<
  decltype(std::declval<T &>().allocate(std::declval<std::size_t>())), void *
  >::value>::type> : std::true_type {};

template <typename T, typename = void>
struct has_deallocate : std::false_type {};

template <typename T>
struct has_deallocate<T, 
  decltype(std::declval<T &>().deallocate(std::declval<void *>(), 
                                          std::declval<std::size_t>()))
  > : std::true_type {};

template <typename T>
using is_memory_resource = 
  typename std::enable_if<has_allocate  <T>::value && 
                          has_deallocate<T>::value>::type;

} // namespace detail

/**
 * @brief An allocator which exhibits different allocation behavior 
 * depending upon the memory resource from which it is constructed.
 * 
 * @remarks Unlike std::pmr::polymorphic_allocator, it does not 
 * rely on a specific inheritance relationship and only restricts 
 * the interface behavior of the incoming memory resource object to 
 * conform to std::pmr::memory_resource.
 * 
 * @see https://en.cppreference.com/w/cpp/memory/memory_resource
 *      https://en.cppreference.com/w/cpp/memory/polymorphic_allocator
 */
class LIBIMP_EXPORT allocator {

};

LIBPMR_NAMESPACE_END_
