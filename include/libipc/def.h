/**
 * @file def.h
 * @author mutouyun (orz@orzz.org)
 * @brief Define the trivial configuration information
 * @date 2022-02-27
 */
#pragma once

#include <cstddef>
#include <cstdint>

#if !defined(LIBIPC_NAMESPACE)
# define LIBIPC_NAMESPACE ipc
#endif
#define LIBIPC_NAMESPACE_BEG_ namespace LIBIPC_NAMESPACE {
#define LIBIPC_NAMESPACE_END_ }

LIBIPC_NAMESPACE_BEG_

// constants

LIBIPC_NAMESPACE_END_
