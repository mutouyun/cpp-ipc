
#include <utility>  // std::exchange, std::swap (since C++11)

#include "libimp/log.h"

#include "libimp/detect_plat.h"
#if defined(LIBIMP_OS_WIN)
#include "libipc/platform/win/shm_impl.h"
#else
#include "libipc/platform/posix/shm_impl.h"
#endif

LIBIPC_NAMESPACE_BEG_

/// @brief C style shared memory access interface implementation.

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

std::string shm_name(shm_t h) noexcept {
  LIBIMP_LOG_();
  if (h == nullptr) {
    log.error("shm handle is null.");
    return {};
  }
  auto shm = static_cast<shm_handle *>(h);
  return shm->file;
}

/// @brief The shared memory object.

shared_memory::shared_memory() noexcept
  : shm_(nullptr) {}

shared_memory::~shared_memory() noexcept {
  this->close();
}

shared_memory::shared_memory(shared_memory &&other) noexcept
  : shm_(std::exchange(other.shm_, nullptr)) {}

shared_memory &shared_memory::operator=(shared_memory &&rhs) & noexcept {
  this->shm_ = std::exchange(rhs.shm_, nullptr);
  return *this;
}

void shared_memory::swap(shared_memory &other) noexcept {
  std::swap(this->shm_, other.shm_);
}

shared_memory::shared_memory(std::string name, std::size_t size, mode::type type) noexcept
  : shared_memory() {
  open(std::move(name), size, type);
}

bool shared_memory::open(std::string name, std::size_t size, mode::type type) noexcept {
  this->close();
  auto ret = shm_open(std::move(name), size, type);
  if (!ret) return false;
  shm_ = *ret;
  return true;
}

void shared_memory::close() noexcept {
  if (!valid()) return;
  shm_close(std::exchange(shm_, nullptr));
}

bool shared_memory::valid() const noexcept {
  return this->shm_ != nullptr;
}

void *shared_memory::get() const noexcept {
  return shm_get(shm_);
}

void *shared_memory::operator*() const noexcept {
  return this->get();
}

std::size_t shared_memory::size() const noexcept {
  return shm_size(shm_);
}

std::string shared_memory::name() const noexcept {
  return shm_name(shm_);
}

LIBIPC_NAMESPACE_END_
