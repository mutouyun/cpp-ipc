#pragma once

#include <thread>             // std::thread
#include <mutex>              // std::mutex
#include <condition_variable> // std::condition_variable
#include <deque>              // std::deque
#include <functional>         // std::function
#include <utility>            // std::forward, std::move
#include <cstddef>            // std::size_t
#include <cassert>            // assert

#include "capo/scope_guard.hpp"

namespace ipc_ut {

class thread_pool final {

  std::deque<std::thread> workers_;
  std::deque<std::function<void()>> jobs_;

  std::mutex lock_;
  std::condition_variable cv_jobs_;
  std::condition_variable cv_empty_;

  std::size_t waiting_cnt_ = 0;
  bool quit_ = false;

  static void proc(thread_pool * pool) {
    assert(pool != nullptr);
    std::function<void()> job;
    for (;;) {
      {
        std::unique_lock<std::mutex> guard { pool->lock_ };
        if (pool->quit_) return;
        if (pool->jobs_.empty()) {
          pool->waiting_cnt_ += 1;
          CAPO_SCOPE_GUARD_ = [pool] {
              pool->waiting_cnt_ -= 1;
          };

          if (pool->waiting_cnt_ == pool->workers_.size()) {
            pool->cv_empty_.notify_all();
          }
          assert(pool->waiting_cnt_ <= pool->workers_.size());
          do {
            pool->cv_jobs_.wait(guard);
            if (pool->quit_) return;
          } while (pool->jobs_.empty());
        }
        assert(!pool->jobs_.empty());
        job = std::move(pool->jobs_.front());
        pool->jobs_.pop_front();
      }
      if (job) job();
    }
  }

public:
  thread_pool() = default;

  ~thread_pool() {
    {
      std::lock_guard<std::mutex> guard { lock_ };
      static_cast<void>(guard);
      quit_ = true;
    }
    cv_jobs_.notify_all();
    cv_empty_.notify_all();
    for (auto & trd : workers_) trd.join();
  }

  explicit thread_pool(std::size_t n) : thread_pool() {
    start(n);
  }

  void start(std::size_t n) {
    std::unique_lock<std::mutex> guard { lock_ };
    if (n <= workers_.size()) return;
    for (std::size_t i = workers_.size(); i < n; ++i) {
      workers_.push_back(std::thread { &thread_pool::proc, this });
    }
  }

  std::size_t size() const noexcept {
    return workers_.size();
  }

  std::size_t jobs_size() const noexcept {
    return jobs_.size();
  }

  void wait_for_started() {
    std::unique_lock<std::mutex> guard { lock_ };
    if (quit_) return;
    while (!workers_.empty() && (waiting_cnt_ != workers_.size())) {
      cv_empty_.wait(guard);
      if (quit_) return;
    }
  }

  void wait_for_done() {
    std::unique_lock<std::mutex> guard { lock_ };
    if (quit_) return;
    while (!jobs_.empty() || (waiting_cnt_ != workers_.size())) {
      assert(waiting_cnt_ <= workers_.size());
      cv_empty_.wait(guard);
      if (quit_) return;
    }
  }

  template <typename F>
  thread_pool & operator<<(F && job) {
    {
      std::lock_guard<std::mutex> guard { lock_ };
      static_cast<void>(guard);
      jobs_.emplace_back(std::forward<F>(job));
    }
    cv_jobs_.notify_one();
    return *this;
  }
};

} // namespace ipc_ut