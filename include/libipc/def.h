/**
 * @file libipc/def.h
 * @author mutouyun (orz@orzz.org)
 * @brief Define the trivial configuration information
 * @date 2022-02-27
 */
#pragma once

#include <cstddef>
#include <cstdint>

#define LIBIPC_               ipc
#define LIBIPC_NAMESPACE_     LIBIPC_
#define LIBIPC_NAMESPACE_BEG_ namespace LIBIPC_NAMESPACE_ {
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

LIBIPC_NAMESPACE_END_
