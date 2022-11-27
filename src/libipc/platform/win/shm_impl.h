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
using namespace ::LIBIMP;

result<shm_t> shm_open(std::string name, std::size_t size, mode::type type) noexcept {
  LIBIMP_LOG_();
  auto h = mmap_open(name, size, type);
  if (h == NULL) {
    log.error("failed: mmap_open(name = {}, size = {}, type = {}).", name, size, type);
    return {nullptr, h.code_value()};
  }
  auto mem = mmap_memof(*h);
  if (*mem == NULL) {
    log.error("failed: mmap_memof({}).", *h);
    mmap_close(*h);
    return {nullptr, mem.code_value()};
  }
  auto sz = mmap_sizeof(*mem);
  if (!sz) {
    log.error("failed: mmap_sizeof({}).", *mem);
    mmap_close(*h);
    return {nullptr, static_cast<result_type>(sz.value())};
  }
  return new shm_handle{std::move(name), *sz, *mem, *h};
}

result_code shm_close(shm_t h) noexcept {
  LIBIMP_LOG_();
  if (h == nullptr) {
    log.error("shm handle is null.");
    return {};
  }
  auto shm = static_cast<shm_handle *>(h);
  auto ret = mmap_release(shm->h_fmap, shm->memp);
  delete shm;
  return ret;
}

LIBIPC_NAMESPACE_END_
