/**
 * @file libpmr/allocator.h
 * @author mutouyun (orz@orzz.org)
 * @brief A generic polymorphic memory allocator.
 * @date 2022-11-13
 */
#pragma once

#include "libpmr/def.h"
#include "libpmr/memory_resource.h"

LIBPMR_NAMESPACE_BEG_

/**
 * @brief An allocator which exhibits different allocation behavior 
 * depending upon the memory resource from which it is constructed.
 * 
 * @remarks Unlike std::pmr::polymorphic_allocator, it does not 
 * rely on a specific inheritance relationship and only restricts 
 * the interface behavior of the incoming memory resource object to 
 * conform to std::pmr::memory_resource.
 * 
 * @see https://en.cppreference.com/w/cpp/memory/memory_resource
 *      https://en.cppreference.com/w/cpp/memory/polymorphic_allocator
 */


LIBPMR_NAMESPACE_END_
