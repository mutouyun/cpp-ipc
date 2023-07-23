/**
 * \file libipc/platform/posix/shm_impl.h
 * \author mutouyun (orz@orzz.org)
 */
#pragma once

#include <string>
#include <memory>   // std::unique_ptr
#include <cstddef>

#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "libimp/log.h"
#include "libimp/system.h"
#include "libimp/detect_plat.h"

#include "libipc/shm.h"

#include "def.h"

LIBIPC_NAMESPACE_BEG_
using namespace ::LIBIMP;

struct shm_handle {
  std::string file;
  std::size_t f_sz;
  void *memp;
};

namespace {

shm_handle *valid(shm_t h) noexcept {
  LIBIMP_LOG_();
  if (h == nullptr) {
    log.error("shm handle is null.");
    return nullptr;
  }
  auto shm = static_cast<shm_handle *>(h);
  if (shm->memp == nullptr) {
    log.error("memory pointer is null.");
    return nullptr;
  }
  return shm;
}

result<int> shm_open_fd(std::string const &name, mode::type type) noexcept {
  LIBIMP_LOG_();
  if (name.empty()) {
    log.error("name is empty.");
    return {};
  }

  /// \brief Open the object for read-write access.
  int flag = O_RDWR;
  switch (type) {
  case mode::open:
    break;
  // The check for the existence of the object, 
  // and its creation if it does not exist, are performed atomically.
  case mode::create:
    flag |= O_CREAT | O_EXCL;
    break;
  // Create the shared memory object if it does not exist.
  case mode::open | mode::create:
    flag |= O_CREAT;
    break;
  default:
    log.error("mode type is invalid. type = ", type);
    return {};
  }

  /// \brief Create/Open POSIX shared memory bject
  int fd = ::shm_open(name.c_str(), flag, S_IRUSR | S_IWUSR |
                                          S_IRGRP | S_IWGRP |
                                          S_IROTH | S_IWOTH);
  if (fd == posix::failed) {
    auto err = sys::error();
    log.error("failed: shm_open(name = ", name, 
                             ", type = ", type, 
                           "). error = ", err);
    return err;
  }
  return fd;
}

result_code ftruncate_fd(int fd, std::size_t size) noexcept {
  LIBIMP_LOG_();
  /// \see https://man7.org/linux/man-pages/man3/ftruncate.3p.html
  if (::ftruncate(fd, size) != posix::succ) {
    auto err = sys::error();
    log.error("failed: ftruncate(", fd, ", ", size, "). error = ", err);
    return err;
  }
  return posix::succ;
}

} // namespace

/**
 * \see https://man7.org/linux/man-pages/man3/shm_open.3.html
 *      https://man7.org/linux/man-pages/man3/fstat.3p.html
 *      https://man7.org/linux/man-pages/man2/mmap.2.html
 */
result<shm_t> shm_open(std::string name, std::size_t size, mode::type type) noexcept {
  LIBIMP_LOG_();
  auto fd = shm_open_fd(name, type);
  if (!fd) {
    return fd.error();
  }
  LIBIMP_UNUSED auto guard = std::unique_ptr<decltype(fd), void (*)(decltype(fd) *)> {
    &fd, [](decltype(fd) *pfd) {
      if (pfd != nullptr) ::close(**pfd);
    }};

  /// \brief Try to get the size of this fd
  struct stat st;
  if (::fstat(*fd, &st) == posix::failed) {
    auto err = sys::error();
    log.error("failed: fstat(fd = ", *fd, "). error = ", err);
    return err;
  }

  /// \brief Truncate this fd to a specified length
  if (size == 0) {
    size = static_cast<std::size_t>(st.st_size);
    auto ret = ftruncate_fd(*fd, size);
    if (!ret) return ret.error();
  } else if (st.st_size > 0) {
    /// \remark Based on the actual size.
    size = static_cast<std::size_t>(st.st_size);
  } else { // st.st_size <= 0
    auto ret = ftruncate_fd(*fd, size);
    if (!ret) return ret.error();
  }

  /// \brief Creates a new mapping in the virtual address space of the calling process.
  void *mem = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, *fd, 0);
  if (mem == MAP_FAILED) {
    auto err = sys::error();
    log.error("failed: mmap(size = ", size, ", fd = ", *fd, "). error = ", err);
    return err;
  }
  return new shm_handle{std::move(name), size, mem};
}

/**
 * \see https://man7.org/linux/man-pages/man2/mmap.2.html
 */
result<void> shm_close(shm_t h) noexcept {
  LIBIMP_LOG_();
  auto shm = valid(h);
  if (shm == nullptr) return {};
  if (::munmap(shm->memp, shm->f_sz) == posix::failed) {
    auto err = sys::error();
    log.error("failed: munmap(", shm->memp, ", ", shm->f_sz, "). error = ", err);
    return err;
  }
  /// \brief no unlink the file.
  delete shm;
  return no_error;
}

LIBIPC_NAMESPACE_END_
