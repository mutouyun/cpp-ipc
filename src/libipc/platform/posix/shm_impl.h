/**
 * @file libipc/platform/posix/shm_impl.h
 * @author mutouyun (orz@orzz.org)
 */
#pragma once

#include <string>
#include <cstddef>

#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "libimp/log.h"
#include "libimp/system.h"

#include "libipc/shm.h"

#include "def.h"

LIBIPC_NAMESPACE_BEG_
using namespace ::LIBIMP_;

struct shm_handle {
  std::string file;
  std::size_t f_sz;
  void *memp;
};

/**
 * @see https://man7.org/linux/man-pages/man3/shm_open.3.html
 *      https://man7.org/linux/man-pages/man3/fstat.3p.html
 *      https://man7.org/linux/man-pages/man3/ftruncate.3p.html
 *      https://man7.org/linux/man-pages/man2/mmap.2.html
 */
result<shm_t> shm_open(std::string name, std::size_t size, mode::type type) noexcept {
  LIBIMP_LOG_();
  if (name.empty()) {
    log.error("name is empty.");
    return {};
  }

  /// @brief Open the object for read-write access.
  int flag = O_RDWR;
  switch (type) {
  case mode::open:
    size = 0;
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
    log.error("mode type is invalid. type = {}", type);
    return {};
  }

  /// @brief Create/Open POSIX shared memory bject
  int fd = ::shm_open(name.c_str(), flag, S_IRUSR | S_IWUSR |
                                          S_IRGRP | S_IWGRP |
                                          S_IROTH | S_IWOTH);
  if (fd == posix::failed) {
    log.error("shm_open fails. error = {}", sys::error_msg(sys::error_code()));
    return {};
  }

  if (size == 0) {
    /// @brief Try to get the size of this fd
    struct stat st;
    if (::fstat(fd, &st) == posix::failed) {
      log.error("fstat fails. error = {}", sys::error_msg(sys::error_code()));
      ::shm_unlink(name.c_str());
      return {};
    }
    size = static_cast<std::size_t>(st.st_size);
    /// @brief Truncate this fd to a specified length
  } else if (::ftruncate(fd, size) != 0) {
    log.error("ftruncate fails. error = {}", sys::error_msg(sys::error_code()));
    ::shm_unlink(name.c_str());
    return {};
  }

  /// @brief Creates a new mapping in the virtual address space of the calling process.
  void *mem = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (mem == MAP_FAILED) {
    log.error("mmap fails. error = {}", sys::error_msg(sys::error_code()));
    ::shm_unlink(name.c_str());
    return {};
  }
  ::close(fd);
  return new shm_handle{std::move(name), size, mem};
}

/**
 * @see https://man7.org/linux/man-pages/man2/mmap.2.html
 */
result_code shm_close(shm_t h) noexcept {
  LIBIMP_LOG_();
  if (h == nullptr) {
    log.error("shm handle is null.");
    return {};
  }
  auto shm = static_cast<shm_handle *>(h);
  if (shm->memp == nullptr) {
    log.error("memory pointer is null.");
    return {};
  }
  if (::munmap(shm->memp, shm->f_sz) == posix::failed) {
    auto ec = sys::error_code();
    log.error("munmap fails. error = {}", sys::error_msg(ec));
    return ec;
  }
  /// @brief no unlink the file.
  delete shm;
  return {true};
}

LIBIPC_NAMESPACE_END_