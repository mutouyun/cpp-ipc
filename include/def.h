#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <thread>

namespace ipc {

// types

using byte_t = std::uint8_t;

template <std::size_t N>
struct uint;

template <> struct uint<8 > { using type = std::uint8_t ; };
template <> struct uint<16> { using type = std::uint16_t; };
template <> struct uint<32> { using type = std::uint32_t; };

template <std::size_t N>
using uint_t = typename uint<N>::type;

// constants

enum : std::size_t {
    error_count = std::numeric_limits<std::size_t>::max(),
    data_length = 16
};

} // namespace ipc

////////////////////////////////////////////////////////////////
/// Gives hint to processor that improves performance of spin-wait loops.
////////////////////////////////////////////////////////////////

#pragma push_macro("IPC_LOCK_PAUSE_")
#undef  IPC_LOCK_PAUSE_

#if defined(_MSC_VER)
#include <windows.h>    // YieldProcessor
/*
    See: http://msdn.microsoft.com/en-us/library/windows/desktop/ms687419(v=vs.85).aspx
    Not for intel c++ compiler, so ignore http://software.intel.com/en-us/forums/topic/296168
*/
#   define IPC_LOCK_PAUSE_() YieldProcessor()
#elif defined(__GNUC__)
#if defined(__i386__) || defined(__x86_64__)
/*
    See: Intel(R) 64 and IA-32 Architectures Software Developer's Manual V2
         PAUSE-Spin Loop Hint, 4-57
         http://www.intel.com/content/www/us/en/architecture-and-technology/64-ia-32-architectures-software-developer-instruction-set-reference-manual-325383.html?wapkw=instruction+set+reference
*/
#   define IPC_LOCK_PAUSE_() __asm__ __volatile__("pause")
#elif defined(__ia64__) || defined(__ia64)
/*
    See: Intel(R) Itanium(R) Architecture Developer's Manual, Vol.3
         hint - Performance Hint, 3:145
         http://www.intel.com/content/www/us/en/processors/itanium/itanium-architecture-vol-3-manual.html
*/
#   define IPC_LOCK_PAUSE_() __asm__ __volatile__ ("hint @pause")
#elif defined(__arm__)
/*
    See: ARM Architecture Reference Manuals (YIELD)
         http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.subset.architecture.reference/index.html
*/
#   define IPC_LOCK_PAUSE_() __asm__ __volatile__ ("yield")
#endif
#endif/*compilers*/

#if !defined(IPC_LOCK_PAUSE_)
/*
    Just use a compiler fence, prevent compiler from optimizing loop
*/
#   define IPC_LOCK_PAUSE_() std::atomic_signal_fence(std::memory_order_seq_cst)
#endif/*!defined(IPC_LOCK_PAUSE_)*/

////////////////////////////////////////////////////////////////
/// Yield to other threads
////////////////////////////////////////////////////////////////

namespace ipc {

inline void yield(unsigned k) {
    if (k < 4)  { /* Do nothing */ }
    else
    if (k < 16) { IPC_LOCK_PAUSE_(); }
    else
    { std::this_thread::yield(); }
}

} // namespace ipc

#pragma pop_macro("IPC_LOCK_PAUSE_")
