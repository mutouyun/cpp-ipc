/**
 * \file libipc/export.h
 * \author mutouyun (orz@orzz.org)
 * \brief Define the symbol export interfaces.
 */
#pragma once

#include "libipc/imp/detect_plat.h"

#if defined(Q_DECL_EXPORT) && defined(Q_DECL_IMPORT)

# define LIBIPC_DECL_EXPORT Q_DECL_EXPORT
# define LIBIPC_DECL_IMPORT Q_DECL_IMPORT

#else // defined(Q_DECL_EXPORT) && defined(Q_DECL_IMPORT)

/**
 * \brief Compiler & system detection for LIBIPC_DECL_EXPORT & LIBIPC_DECL_IMPORT.
 * Not using QtCore cause it shouldn't depend on Qt.
 */
# if defined(LIBIPC_CC_MSVC) || defined(LIBIPC_OS_WIN)
#   define LIBIPC_DECL_EXPORT __declspec(dllexport)
#   define LIBIPC_DECL_IMPORT __declspec(dllimport)
# elif defined(LIBIPC_OS_ANDROID) || defined(LIBIPC_OS_LINUX) || defined(LIBIPC_CC_GNUC)
#   define LIBIPC_DECL_EXPORT __attribute__((visibility("default")))
#   define LIBIPC_DECL_IMPORT __attribute__((visibility("default")))
# else
#   define LIBIPC_DECL_EXPORT __attribute__((visibility("default")))
#   define LIBIPC_DECL_IMPORT __attribute__((visibility("default")))
# endif

#endif // defined(Q_DECL_EXPORT) && defined(Q_DECL_IMPORT)

/**
 * \brief Define LIBIPC_EXPORT for exporting function & class.
 */
#ifndef LIBIPC_EXPORT
# if defined(LIBIPC_LIBRARY_SHARED_BUILDING__)
#   define LIBIPC_EXPORT LIBIPC_DECL_EXPORT
# elif defined(LIBIPC_LIBRARY_SHARED_USING__)
#   define LIBIPC_EXPORT LIBIPC_DECL_IMPORT
# else
#   define LIBIPC_EXPORT
# endif
#endif /*LIBIPC_EXPORT*/
