/**
 * @file libipc/shm.h
 * @author mutouyun (orz@orzz.org)
 * @brief Define the shared memory access interface
 * @date 2022-04-17
 */
#pragma once

#include <string>
#include <cstddef>

#include "libimp/export.h"
#include "libimp/result.h"

#include "libipc/def.h"

LIBIPC_NAMESPACE_BEG_

struct shm_handle;
using shm_t = shm_handle *;

/// @brief Create a new shared memory handle with a name of the shared memory file.
LIBIMP_EXPORT ::LIBIMP_::result<shm_t> shm_open(std::string name, 
                                                std::size_t size = 0,
                                                mode::type = mode::create | mode::open) noexcept;

/// @brief Close the shared memory handle.
LIBIMP_EXPORT ::LIBIMP_::result_code shm_close(shm_t) noexcept;

/// @brief Gets a memory pointer based on the shared memory handle.
/// @return nullptr on failure.
LIBIMP_EXPORT void *shm_get(shm_t) noexcept;

/// @brief Gets the memory size based on the shared memory handle.
/// @return 0 on failure.
LIBIMP_EXPORT std::size_t shm_size(shm_t) noexcept;

/// @brief Gets the name of the shared memory file based on the shared memory handle.
/// @return empty string on failure.
LIBIMP_EXPORT std::string shm_name(shm_t) noexcept;

/**
 * @brief The shared memory object.
 */
class LIBIMP_EXPORT shared_memory {
  shm_t shm_;

public:
  explicit shared_memory() noexcept;
  ~shared_memory() noexcept;

  shared_memory(shared_memory &&other) noexcept;
  shared_memory &operator=(shared_memory &&rhs) & noexcept;
  void swap(shared_memory &other) noexcept;

  explicit shared_memory(std::string name, 
                         std::size_t size = 0, 
                         mode::type = mode::create | mode::open) noexcept;

  bool open(std::string name, std::size_t size, mode::type) noexcept;
  void close() noexcept;
  bool valid() const noexcept;

  void *get() const noexcept;
  void *operator*() const noexcept;
  template <typename T>
  T *as() { return static_cast<T *>(get()); }

  std::size_t size() const noexcept;
  std::string name() const noexcept;
};

LIBIPC_NAMESPACE_END_
