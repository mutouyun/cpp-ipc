/**
 * @file spin_lock.h
 * @author mutouyun (orz@orzz.org)
 * @brief Define spin locks
 * @date 2022-02-27
 */
#pragma once

#include <atomic>
#include <thread>
#include <chrono>
#include <limits>
#include <type_traits>
#include <utility>

#include "libipc/detect_plat.h"
#include "libipc/def.h"

/**
 * @brief Gives hint to processor that improves performance of spin-wait loops.
*/

#pragma push_macro("LIBIPC_LOCK_PAUSE_")
#undef LIBIPC_LOCK_PAUSE_

#if defined(LIBIPC_CC_MSVC)
# include <Windows.h>   // YieldProcessor
/**
 * @brief Not for intel c++ compiler, so ignore http://software.intel.com/en-us/forums/topic/296168
 * @see http://msdn.microsoft.com/en-us/library/windows/desktop/ms687419(v=vs.85).aspx
*/
# define LIBIPC_LOCK_PAUSE_() YieldProcessor()
#elif defined(LIBIPC_CC_GNUC)
# if defined(LIBIPC_INSTR_X86_64)
/**
 * @brief Intel(R) 64 and IA-32 Architectures Software Developer's Manual V2
 *        PAUSE-Spin Loop Hint, 4-57
 * @see http://www.intel.com/content/www/us/en/architecture-and-technology/64-ia-32-architectures-software-developer-instruction-set-reference-manual-325383.html?wapkw=instruction+set+reference
*/
#   define LIBIPC_LOCK_PAUSE_() __asm__ __volatile__("pause")
# elif defined(LIBIPC_INSTR_I64)
/**
 * @brief Intel(R) Itanium(R) Architecture Developer's Manual, Vol.3
 *        hint - Performance Hint, 3:145
 * @see http://www.intel.com/content/www/us/en/processors/itanium/itanium-architecture-vol-3-manual.html
*/
#   define LIBIPC_LOCK_PAUSE_() __asm__ __volatile__ ("hint @pause")
# elif defined(LIBIPC_INSTR_ARM)
/**
 * @brief ARM Architecture Reference Manuals (YIELD)
 * @see http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.subset.architecture.reference/index.html
*/
#   define LIBIPC_LOCK_PAUSE_() __asm__ __volatile__ ("yield")
# endif
#endif /*compilers*/

#if !defined(LIBIPC_LOCK_PAUSE_)
/**
 * @brief Just use a compiler fence, prevent compiler from optimizing loop
*/
# define LIBIPC_LOCK_PAUSE_() std::atomic_signal_fence(std::memory_order_seq_cst)
#endif /*!defined(LIBIPC_LOCK_PAUSE_)*/

LIBIPC_NAMESPACE_BEG_

/**
 * @brief Yield to other threads
*/

template <typename K>
inline void yield(K &k) noexcept {
  if (k < 4)  { /* Do nothing */ }
  else
  if (k < 16) { LIBIPC_LOCK_PAUSE_(); }
  else
  if (k < 32) { std::this_thread::yield(); }
  else {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return;
  }
  ++k;
}

template <std::size_t N = 32, typename K, typename F>
inline void sleep(K &k, F &&f) {
  if (k < static_cast<K>(N)) {
    std::this_thread::yield();
  }
  else {
    static_cast<void>(std::forward<F>(f)());
    return;
  }
  ++k;
}

template <std::size_t N = 32, typename K>
inline void sleep(K &k) {
  sleep<N>(k, [] {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  });
}

/**
 * @brief Basic spin lock
*/
class spin_lock {
  std::atomic<unsigned> lc_ {0};

public:
  void lock(void) noexcept {
    for (unsigned k = 0;
         lc_.exchange(1, std::memory_order_acquire);
         yield(k)) ;
  }
  void unlock(void) noexcept {
    lc_.store(0, std::memory_order_release);
  }
};

/**
 * @brief Support for shared mode spin lock
*/
class rw_lock {
  using lc_ui_t = unsigned;

  std::atomic<lc_ui_t> lc_ {0};

  enum : lc_ui_t {
    w_mask = (std::numeric_limits<std::make_signed_t<lc_ui_t>>::max)(), // b 0111 1111
    w_flag = w_mask + 1,                                                // b 1000 0000
  };

public:
  rw_lock() noexcept = default;

  rw_lock(const rw_lock &) = delete;
  rw_lock &operator=(const rw_lock &) = delete;
  rw_lock(rw_lock &&) = delete;
  rw_lock &operator=(rw_lock &&) = delete;

  void lock() noexcept {
    for (unsigned k = 0;;) {
      auto old = lc_.fetch_or(w_flag, std::memory_order_acq_rel);
      if (!old) return;           // got w-lock
      if (!(old & w_flag)) break; // other thread having r-lock
      yield(k);                   // other thread having w-lock
    }
    // wait for reading finished
    for (unsigned k = 0;
         lc_.load(std::memory_order_acquire) & w_mask;
         yield(k)) ;
  }

  void unlock() noexcept {
    lc_.store(0, std::memory_order_release);
  }

  void lock_shared() noexcept {
    auto old = lc_.load(std::memory_order_acquire);
    for (unsigned k = 0;;) {
      // if w_flag set, just continue
      if (old & w_flag) {
        yield(k);
        old = lc_.load(std::memory_order_acquire);
      }
      // otherwise try cas lc + 1 (set r-lock)
      else if (lc_.compare_exchange_weak(old, old + 1, std::memory_order_release)) {
        return;
      }
      // set r-lock failed, old has been updated
    }
  }

  void unlock_shared() noexcept {
    lc_.fetch_sub(1, std::memory_order_release);
  }
};

LIBIPC_NAMESPACE_END_

#pragma pop_macro("LIBIPC_LOCK_PAUSE_")
