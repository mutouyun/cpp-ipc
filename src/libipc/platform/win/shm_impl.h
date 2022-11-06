/**
 * @file libipc/platform/win/shm_impl.h
 * @author mutouyun (orz@orzz.org)
 */
#pragma once

#include <string>
#include <cstddef>

#include "libimp/log.h"
#include "libimp/system.h"

#include "libipc/shm.h"

#include "mmap_impl.h"

LIBIPC_NAMESPACE_BEG_
using namespace ::LIBIMP_;

result<shm_t> shm_open(std::string name, std::size_t size, mode::type type) noexcept {
  LIBIMP_LOG_();
  auto h = mmap_open(name, size, type);
  if (h == NULL) {
    log.error("mmap_open failed.");
    return {nullptr, *sys::error_code()};
  }
  auto mem = mmap_memof(h);
  if (mem == NULL) {
    log.warning("mmap_memof failed.");
  }
  return new shm_handle{std::move(name), mmap_sizeof(mem), mem, h};
}

result_code shm_close(shm_t h) noexcept {
  LIBIMP_LOG_();
  if (h == nullptr) {
    log.error("shm handle is null.");
    return {};
  }
  auto shm = static_cast<shm_handle *>(h);
  mmap_release(shm->h_fmap, shm->memp);
  delete shm;
  return {0};
}

LIBIPC_NAMESPACE_END_
