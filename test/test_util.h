
#pragma once

#include "libimp/detect_plat.h"
#ifndef LIBIMP_OS_WIN
# include <sys/wait.h>
# include <unistd.h>
#else
# define pid_t int
#endif

#include <condition_variable>
#include <mutex>

namespace test {

template <typename Fn>
pid_t subproc(Fn&& fn) {
#ifndef LIBIMP_OS_WIN
  pid_t pid = fork();
  if (pid == -1) {
    return pid;
  }
  if (!pid) {
    // Unhook doctest from the subprocess.
    // Otherwise, we would see a test-failure printout after the crash.
    signal(SIGABRT, SIG_DFL);
    signal(SIGSEGV, SIG_DFL);
    fn();
    exit(0);
  }
  return pid;
#else
  return -1;
#endif
}

inline void join_subproc(pid_t pid) {
#ifndef LIBIMP_OS_WIN
  int ret_code;
  waitpid(pid, &ret_code, 0);
#endif
}

/// \brief A simple latch implementation.
class latch {
public:
  explicit latch(int count) : count_(count) {}

  void count_down() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (count_ > 0) {
      --count_;
      if (count_ == 0) {
        cv_.notify_all();
      }
    }
  }

  void wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [&] { return count_ == 0; });
  }

  void arrive_and_wait() {
    count_down();
    wait();
  }

private:
  std::mutex mutex_;
  std::condition_variable cv_;
  int count_;
};

} // namespace test

#define REQUIRE_EXIT(...) \
  test::join_subproc(test::subproc([&]() __VA_ARGS__))
