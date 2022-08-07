/**
 * @file libimp/detect_plat.h
 * @author mutouyun (orz@orzz.org)
 * @brief Define platform detection related interfaces
 * @date 2022-02-27
 */
#pragma once

// OS

#if defined(WINCE) || defined(_WIN32_WCE)
# define LIBIMP_OS_WINCE
#elif defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || \
     (defined(__x86_64) && defined(__MSYS__))
#define LIBIMP_OS_WIN64
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || \
      defined(__NT__) || defined(__MSYS__)
# define LIBIMP_OS_WIN32
#elif defined(__QNX__) || defined(__QNXNTO__)
# define LIBIMP_OS_QNX
#elif defined(__APPLE__)
# define LIBIMP_OS_APPLE
#elif defined(ANDROID) || defined(__ANDROID__)
# define LIBIMP_OS_ANDROID
#elif defined(__linux__) || defined(__linux)
# define LIBIMP_OS_LINUX
#else
# error "This OS is unsupported."
#endif

#if defined(LIBIMP_OS_WIN32) || defined(LIBIMP_OS_WIN64) || \
    defined(LIBIMP_OS_WINCE)
# define LIBIMP_OS_WIN
#endif

// Compiler

#if defined(_MSC_VER)
# define LIBIMP_CC_MSVC
#elif defined(__GNUC__)
# define LIBIMP_CC_GNUC
#else
# error "This compiler is unsupported."
#endif

// Instruction set
// @see https://sourceforge.net/p/predef/wiki/Architectures/

#if defined(_M_X64) || defined(_M_AMD64) || \
    defined(__x86_64__) || defined(__x86_64) || \
    defined(__amd64__) || defined(__amd64)
# define LIBIMP_INSTR_X64
#elif defined(_M_IA64) || defined(__IA64__) || defined(_IA64) || \
      defined(__ia64__) || defined(__ia64)
# define LIBIMP_INSTR_I64
#elif defined(_M_IX86) || defined(_X86_) || defined(__i386__) || defined(__i386)
# define LIBIMP_INSTR_X86
#elif defined(_M_ARM64) || defined(__arm64__) || defined(__aarch64__)
# define LIBIMP_INSTR_ARM64
#elif defined(_M_ARM) || defined(_ARM) || defined(__arm__) || defined(__arm)
# define LIBIMP_INSTR_ARM32
#else
# error "This instruction set is unsupported."
#endif

#if defined(LIBIMP_INSTR_X86) || defined(LIBIMP_INSTR_X64)
# define LIBIMP_INSTR_X86_64
#elif defined(LIBIMP_INSTR_ARM32) || defined(LIBIMP_INSTR_ARM64)
# define LIBIMP_INSTR_ARM
#endif

// Byte order

#if defined(__BYTE_ORDER__)
# define LIBIMP_ENDIAN_BIG   (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
# define LIBIMP_ENDIAN_LIT   (!LIBIMP_ENDIAN_BIG)
#else
# define LIBIMP_ENDIAN_BIG   (0)
# define LIBIMP_ENDIAN_LIT   (1)
#endif

// C++ version

#if (__cplusplus >= 202002L) && !defined(LIBIMP_CPP_20)
# define LIBIMP_CPP_20
#endif
#if (__cplusplus >= 201703L) && !defined(LIBIMP_CPP_17)
# define LIBIMP_CPP_17
#endif
#if /*(__cplusplus >= 201402L) &&*/ !defined(LIBIMP_CPP_14)
# define LIBIMP_CPP_14
#endif

#if !defined(LIBIMP_CPP_20) && \
    !defined(LIBIMP_CPP_17) && \
    !defined(LIBIMP_CPP_14)
# error "This C++ version is unsupported."
#endif

// C++ attributes

#if defined(__has_cpp_attribute)
# if __has_cpp_attribute(fallthrough)
#   define LIBIMP_FALLTHROUGH [[fallthrough]]
# endif
# if __has_cpp_attribute(maybe_unused)
#   define LIBIMP_UNUSED [[maybe_unused]]
# endif
# if __has_cpp_attribute(likely)
#   define LIBIMP_LIKELY(...) (__VA_ARGS__) [[likely]]
# endif
# if __has_cpp_attribute(unlikely)
#   define LIBIMP_UNLIKELY(...) (__VA_ARGS__) [[unlikely]]
# endif
#endif

#if !defined(LIBIMP_FALLTHROUGH)
# if defined(LIBIMP_CC_GNUC)
#   define LIBIMP_FALLTHROUGH __attribute__((__fallthrough__))
# else
#   define LIBIMP_FALLTHROUGH
# endif
#endif

#if !defined(LIBIMP_UNUSED)
# if defined(LIBIMP_CC_GNUC)
#   define LIBIMP_UNUSED __attribute__((__unused__))
# elif defined(LIBIMP_CC_MSVC)
#   define LIBIMP_UNUSED __pragma(warning(suppress: 4100 4101 4189))
# else
#   define LIBIMP_UNUSED
# endif
#endif

#if !defined(LIBIMP_LIKELY)
# if defined(__has_builtin)
#   if __has_builtin(__builtin_expect)
#     define LIBIMP_LIKELY(...) (__builtin_expect(!!(__VA_ARGS__), 1))
#   endif
# endif
#endif

#if !defined(LIBIMP_LIKELY)
# define LIBIMP_LIKELY(...) (__VA_ARGS__)
#endif

#if !defined(LIBIMP_UNLIKELY)
# if defined(__has_builtin)
#   if __has_builtin(__builtin_expect)
#     define LIBIMP_UNLIKELY(...) (__builtin_expect(!!(__VA_ARGS__), 0))
#   endif
# endif
#endif

#if !defined(LIBIMP_UNLIKELY)
# define LIBIMP_UNLIKELY(...) (__VA_ARGS__)
#endif
