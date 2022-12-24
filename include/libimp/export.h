/**
 * \file libimp/export.h
 * \author mutouyun (orz@orzz.org)
 * \brief Define the symbol export interfaces
 * \date 2022-02-27
 */
#pragma once

#include "libimp/detect_plat.h"

#if defined(Q_DECL_EXPORT) && defined(Q_DECL_IMPORT)

# define LIBIMP_DECL_EXPORT Q_DECL_EXPORT
# define LIBIMP_DECL_IMPORT Q_DECL_IMPORT

#else // defined(Q_DECL_EXPORT) && defined(Q_DECL_IMPORT)

/**
 * \brief Compiler & system detection for LIBIMP_DECL_EXPORT & LIBIMP_DECL_IMPORT.
 * Not using QtCore cause it shouldn't depend on Qt.
 */
# if defined(LIBIMP_CC_MSVC) || defined(LIBIMP_OS_WIN)
#   define LIBIMP_DECL_EXPORT __declspec(dllexport)
#   define LIBIMP_DECL_IMPORT __declspec(dllimport)
# elif defined(LIBIMP_OS_ANDROID) || defined(LIBIMP_OS_LINUX) || defined(LIBIMP_CC_GNUC)
#   define LIBIMP_DECL_EXPORT __attribute__((visibility("default")))
#   define LIBIMP_DECL_IMPORT __attribute__((visibility("default")))
# else
#   define LIBIMP_DECL_EXPORT __attribute__((visibility("default")))
#   define LIBIMP_DECL_IMPORT __attribute__((visibility("default")))
# endif

#endif // defined(Q_DECL_EXPORT) && defined(Q_DECL_IMPORT)

/**
 * \brief Define LIBIMP_EXPORT for exporting function & class.
 */
#ifndef LIBIMP_EXPORT
# if defined(LIBIMP_LIBRARY_SHARED_BUILDING__)
#   define LIBIMP_EXPORT LIBIMP_DECL_EXPORT
# elif defined(LIBIMP_LIBRARY_SHARED_USING__)
#   define LIBIMP_EXPORT LIBIMP_DECL_IMPORT
# else
#   define LIBIMP_EXPORT
# endif
#endif /*LIBIMP_EXPORT*/
