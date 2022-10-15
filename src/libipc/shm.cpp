
#include "libimp/log.h"

#include "libimp/detect_plat.h"
#if defined(LIBIMP_OS_WIN)
#include "libipc/platform/win/shm_impl.h"
#else
#include "libipc/platform/posix/shm_impl.h"
#endif

LIBIPC_NAMESPACE_BEG_

void *shm_get(shm_t h) noexcept {
  LIBIMP_LOG_();
  if (h == nullptr) {
    log.error("shm handle is null.");
    return {};
  }
  auto shm = static_cast<shm_handle *>(h);
  return shm->memp;
}

std::size_t shm_size(shm_t h) noexcept {
  LIBIMP_LOG_();
  if (h == nullptr) {
    log.error("shm handle is null.");
    return {};
  }
  auto shm = static_cast<shm_handle *>(h);
  return shm->f_sz;
}

std::string shm_file(shm_t h) noexcept {
  LIBIMP_LOG_();
  if (h == nullptr) {
    log.error("shm handle is null.");
    return {};
  }
  auto shm = static_cast<shm_handle *>(h);
  return shm->file;
}

LIBIPC_NAMESPACE_END_
