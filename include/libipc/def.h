/**
 * @file libipc/def.h
 * @author mutouyun (orz@orzz.org)
 * @brief Define the trivial configuration information
 * @date 2022-02-27
 */
#pragma once

#include <cstddef>  // std::max_align_t
#include <cstdint>
#include <new>      // std::hardware_destructive_interference_size

#include "libimp/detect_plat.h"

#define LIBIPC_               ipc
#define LIBIPC_NAMESPACE_BEG_ namespace LIBIPC_ {
#define LIBIPC_NAMESPACE_END_ }

LIBIPC_NAMESPACE_BEG_

// constants

struct prot {
  using type = std::uint32_t;
  enum : type {
    none  = 0x00,
    read  = 0x01,
    write = 0x02,
  };
};

struct mode {
  using type = std::uint32_t;
  enum : type {
    none   = 0x00,
    create = 0x01,
    open   = 0x02,
  };
};

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

LIBIPC_NAMESPACE_END_
