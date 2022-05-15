/**
 * @file libipc/mmap.h
 * @author mutouyun (orz@orzz.org)
 * @brief Define the methods of memory-mapped file I/O
 * @date 2022-04-17
 */
#pragma once

#include "libimp/export.h"
#include "libimp/result.h"

#include "libipc/def.h"

LIBIPC_NAMESPACE_BEG_

struct mmap_handle;
using mmap_t = mmap_handle *;

/**
 * @brief Creates a new mapping in the virtual address space of the calling process.
 */
LIBIMP_EXPORT ::LIBIMP_::result<mmap_t> mmap_open();

LIBIPC_NAMESPACE_END_
