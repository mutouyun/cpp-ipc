
#pragma once

#include <sys/wait.h>
#include <unistd.h>

namespace test {

template <typename Fn>
pid_t subproc(Fn&& fn) {
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
}

inline void join_subproc(pid_t pid) {
  int ret_code;
  waitpid(pid, &ret_code, 0);
}

} // namespace test

#define REQUIRE_EXIT(...) \
  test::join_subproc(test::subproc([&]() __VA_ARGS__))
