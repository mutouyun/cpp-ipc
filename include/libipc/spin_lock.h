/**
 * \file libipc/spin_lock.h
 * \author mutouyun (orz@orzz.org)
 * \brief Define spin locks
 * \date 2022-02-27
 */
#pragma once

#include <atomic>
#include <thread>
#include <chrono>
#include <utility>

#include "libimp/export.h"

#include "libipc/def.h"

LIBIPC_NAMESPACE_BEG_

/**
 * \brief Gives hint to processor that improves performance of spin-wait loops.
*/
LIBIMP_EXPORT void pause() noexcept;

/**
 * \brief Yield to other threads
*/

template <typename K>
inline void yield(K &k) noexcept {
  if (k < 4)  { /* Do nothing */ }
  else
  if (k < 16) { pause(); }
  else
  if (k < 32) { std::this_thread::yield(); }
  else {
    std::this_thread::sleep_for(std::chrono::microseconds(1));
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
    std::this_thread::sleep_for(std::chrono::microseconds(1));
  });
}

/**
 * \brief Basic spin lock
*/
class LIBIMP_EXPORT spin_lock {
  std::atomic<unsigned> lc_ {0};

public:
  void lock() noexcept;
  void unlock() noexcept;
};

/**
 * \brief Support for shared mode spin lock
*/
class LIBIMP_EXPORT rw_lock {
  std::atomic<unsigned> lc_ {0};

public:
  void lock() noexcept;
  void unlock() noexcept;
  void lock_shared() noexcept;
  void unlock_shared() noexcept;
};

LIBIPC_NAMESPACE_END_
