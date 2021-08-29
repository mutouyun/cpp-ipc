#ifndef RDTSC_H
#define RDTSC_H

#include <stdint.h>       // uint64_t

#if defined(_M_X64) || defined(_M_IX86) || defined(__x86_64) || defined(__i386)
# ifdef _WIN32
#  include <intrin.h>     // __rdtsc
# else
#  include <x86intrin.h>  // __rdtsc
# endif
# define HAS_HW_RDTSC 1
#else
# include <chrono>        // std::chrono::high_resolution_clock
# define HAS_HW_RDTSC 0
#endif

inline uint64_t rdtsc()
{
#if HAS_HW_RDTSC
    // _mm_lfence() might be used to serialize the instruction stream,
    // and it would guarantee that RDTSC will not be reordered with
    // other instructions.  However, measurements show that the overhead
    // may be too big (easily 15 to 30 CPU cycles) for profiling
    // purposes: if reordering matters, the overhead matters too!

    // Forbid the compiler from reordering instructions
# ifdef _MSC_VER
    _ReadWriteBarrier();
# else
    __asm__ __volatile__("" : : : "memory");
# endif

    uint64_t result = __rdtsc();

    // Forbid the compiler from reordering instructions
# ifdef _MSC_VER
    _ReadWriteBarrier();
# else
    __asm__ __volatile__("" : : : "memory");
# endif

    return result;
#else
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               now.time_since_epoch())
        .count();
#endif
}

#endif // RDTSC_H
