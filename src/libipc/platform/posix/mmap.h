/**
 * @file libimp/platform/posix/mmap.h
 * @author mutouyun (orz@orzz.org)
 */
#pragma once

#include <string>
#include <cstddef>

#include "libipc/mmap.h"

LIBIPC_NAMESPACE_BEG_

struct mmap_handle {
  std::string file;
  std::size_t f_sz;
  std::size_t f_of;
  void *memp;
};

inline int mmap_open_(mmap_t *h_out, std::string const &file, int fd, int flags, std::size_t f_sz, std::size_t f_of) noexcept {
  if (h_out == nullptr) {
    return -1;
  }
}

LIBIPC_NAMESPACE_END_
