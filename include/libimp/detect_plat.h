/**
 * \file libimp/detect_plat.h
 * \author mutouyun (orz@orzz.org)
 * \brief Define platform detection related interfaces.
 * \date 2022-02-27
 */
#pragma once

/// \brief OS check.

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
#elif defined(_POSIX_VERSION)
# define LIBIMP_OS_POSIX
#else
# error "This OS is unsupported."
#endif

#if defined(LIBIMP_OS_WIN32) || defined(LIBIMP_OS_WIN64) || \
    defined(LIBIMP_OS_WINCE)
# define LIBIMP_OS_WIN
#endif

/// \brief Compiler check.

#if defined(_MSC_VER)
# define LIBIMP_CC_MSVC      _MSC_VER
# define LIBIMP_CC_MSVC_2015 1900
# define LIBIMP_CC_MSVC_2017 1910
# define LIBIMP_CC_MSVC_2019 1920
# define LIBIMP_CC_MSVC_2022 1930
#elif defined(__GNUC__)
# define LIBIMP_CC_GNUC __GNUC__
# if defined(__clang__)
#   define LIBIMP_CC_CLANG
#endif
#else
# error "This compiler is unsupported."
#endif

/// \brief Instruction set.
/// \see https://sourceforge.net/p/predef/wiki/Architectures/

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

/// \brief Byte order.

#if defined(__BYTE_ORDER__)
# define LIBIMP_ENDIAN_BIG   (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
# define LIBIMP_ENDIAN_LIT   (!LIBIMP_ENDIAN_BIG)
#else
# define LIBIMP_ENDIAN_BIG   (0)
# define LIBIMP_ENDIAN_LIT   (1)
#endif

/// \brief C++ version.

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

/// \brief C++ attributes.
/// \see https://en.cppreference.com/w/cpp/language/attributes

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
# if __has_cpp_attribute(nodiscard)
#   define LIBIMP_NODISCARD [[nodiscard]]
# endif
# if __has_cpp_attribute(assume)
#   define LIBIMP_ASSUME(...) [[assume(__VA_ARGS__)]]
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

#if !defined(LIBIMP_NODISCARD)
/// \see https://stackoverflow.com/questions/4226308/msvc-equivalent-of-attribute-warn-unused-result
# if defined(LIBIMP_CC_GNUC) && (LIBIMP_CC_GNUC >= 4)
#   define LIBIMP_NODISCARD __attribute__((warn_unused_result))
# elif defined(LIBIMP_CC_MSVC) && (LIBIMP_CC_MSVC >= 1700)
#   define LIBIMP_NODISCARD _Check_return_
# else
#   define LIBIMP_NODISCARD
# endif
#endif

#if !defined(LIBIMP_ASSUME)
# if defined(__has_builtin)
#   if __has_builtin(__builtin_assume)
      /// \see https://clang.llvm.org/docs/LanguageExtensions.html#langext-builtin-assume
#     define LIBIMP_ASSUME(...) __builtin_assume(__VA_ARGS__)
#   elif __has_builtin(__builtin_unreachable)
      /// \see https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html#index-_005f_005fbuiltin_005funreachable
#     define LIBIMP_ASSUME(...) do { if (!(__VA_ARGS__)) __builtin_unreachable(); } while (false)
#   endif
# endif
#endif

#if !defined(LIBIMP_ASSUME)
# if defined(LIBIMP_CC_MSVC)
    /// \see https://learn.microsoft.com/en-us/cpp/intrinsics/assume?view=msvc-140
#   define LIBIMP_ASSUME(...) __assume(__VA_ARGS__)
# else
#   define LIBIMP_ASSUME(...)
# endif
#endif

/// \see https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_exceptions.html
///      https://learn.microsoft.com/en-us/cpp/preprocessor/predefined-macros
///      https://stackoverflow.com/questions/6487013/programmatically-determine-whether-exceptions-are-enabled
#if defined(__cpp_exceptions) && __cpp_exceptions || \
    defined(__EXCEPTIONS) || defined(_CPPUNWIND)
# define LIBIMP_TRY                    try
# define LIBIMP_CATCH(...)             catch (__VA_ARGS__)
# define LIBIMP_THROW($exception, ...) throw $exception
#else
# define LIBIMP_TRY                    if (true)
# define LIBIMP_CATCH(...)             else if (false)
# define LIBIMP_THROW($exception, ...) return __VA_ARGS__
#endif
