/**
 * @file libconcur/def.h
 * @author mutouyun (orz@orzz.org)
 * @brief Define the trivial configuration information for concurrency.
 * @date 2022-11-07
 */
#pragma once

#include <cstddef>  // std::max_align_t
#include <new>      // std::hardware_destructive_interference_size

#include "libimp/detect_plat.h"

#define LIBCONCUR                concur
#define LIBCONCUR_NAMESPACE_BEG_ namespace LIBCONCUR {
#define LIBCONCUR_NAMESPACE_END_ }

LIBCONCUR_NAMESPACE_BEG_

/// @brief Constants.

enum : std::size_t {
  /// @brief Minimum offset between two objects to avoid false sharing.
  /// @see https://en.cppreference.com/w/cpp/thread/hardware_destructive_interference_size
  cache_line_size =
#if defined(LIBIMP_CPP_17) && defined(__cpp_lib_hardware_interference_size)
    ( std::hardware_destructive_interference_size < alignof(std::max_align_t) ) ? 64 
    : std::hardware_destructive_interference_size,
#else
    64,
#endif
};

LIBCONCUR_NAMESPACE_END_
