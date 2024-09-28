/**
 * \file libipc/platform/win/shm_impl.h
 * \author mutouyun (orz@orzz.org)
 */
#pragma once

#include <string>
#include <cstddef>

#include "libimp/log.h"
#include "libimp/system.h"
#include "libpmr/new.h"
#include "libipc/shm.h"

#include "api.h"

LIBIPC_NAMESPACE_BEG_

using namespace ::LIBIMP;
using namespace ::LIBPMR;

struct shm_handle {
  std::string file;
  std::size_t f_sz;
  void *memp;
  HANDLE h_fmap;
};

result<shm_t> shm_open(std::string name, std::size_t size, mode::type type) noexcept {
  LIBIMP_LOG_();
  auto h = winapi::open_file_mapping(name, size, type);
  if (*h == NULL) {
    log.error("failed: OpenFileMapping(name = ", name, ", size = ", size, ", type = ", type, ").");
    return h.error();
  }
  auto mem = winapi::address_of_file_mapping(*h);
  if (*mem == NULL) {
    log.error("failed: MapViewOfFile(", *h, ").");
    winapi::close_handle(*h);
    return mem.error();
  }
  auto sz = winapi::region_size_of_address(*mem);
  if (!sz) {
    log.error("failed: RegionSizeOfMemory(", *mem, ").");
    winapi::close_handle(*h);
    return sz.error();
  }
  return $new<shm_handle>(std::move(name), *sz, *mem, *h);
}

result<void> shm_close(shm_t h) noexcept {
  LIBIMP_LOG_();
  if (h == nullptr) {
    log.error("shm handle is null.");
    return std::make_error_code(std::errc::invalid_argument);
  }
  auto shm = static_cast<shm_handle *>(h);
  auto ret = winapi::close_file_mapping(shm->h_fmap, shm->memp);
  $delete(shm);
  return ret;
}

LIBIPC_NAMESPACE_END_
