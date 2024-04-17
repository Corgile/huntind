//
// hound-torch / cancelable_sleeper.hpp
// Created by brian on 2024 Apr 17.
//

#ifndef CANCELABLE_SLEEPER_HPP
#define CANCELABLE_SLEEPER_HPP
#include <chrono>
#include <mutex>
#include <condition_variable>

class InterruptibleSleep {
  std::mutex mtx;
  std::condition_variable cv;
  bool stop_requested = false;
public:
  InterruptibleSleep() = default;

  void sleep_for(std::chrono::seconds duration) {
    std::unique_lock lock(mtx);
    cv.wait_for(lock, duration, [this] { return stop_requested; });
  }

  void stop_sleep() {
    std::lock_guard lock(mtx);
    stop_requested = true;
    cv.notify_one();
  }
};

#endif //CANCELABLE_SLEEPER_HPP
