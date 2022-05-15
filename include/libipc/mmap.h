/**
 * @file libipc/mmap.h
 * @author mutouyun (orz@orzz.org)
 * @brief Define the methods of memory-mapped file I/O
 * @date 2022-04-17
 */
#pragma once

#include <string>
#include <cstddef>

#include "libimp/export.h"
#include "libimp/result.h"

#include "libipc/def.h"

LIBIPC_NAMESPACE_BEG_

struct mmap_handle;
using mmap_t = mmap_handle *;

/**
 * @brief Creates a new mapping in the virtual address space of the calling process.
 */
LIBIMP_EXPORT ::LIBIMP_::result<mmap_t> mmap_open(std::string const &file, 
                                                  std::size_t size = 0,
                                                  prot::type = prot::read | prot::write) noexcept;

/**
 * @brief Close the memory mapping handle.
 */
LIBIMP_EXPORT ::LIBIMP_::result_code mmap_close(mmap_t) noexcept;

/**
 * @brief Synchronize a file with a memory map.
 */
LIBIMP_EXPORT ::LIBIMP_::result_code mmap_sync(mmap_t) noexcept;

LIBIMP_EXPORT void *      mmap_get (mmap_t) noexcept;
LIBIMP_EXPORT std::size_t mmap_size(mmap_t) noexcept;
LIBIMP_EXPORT std::string mmap_file(mmap_t) noexcept;

LIBIPC_NAMESPACE_END_
