//
// hound-torch / timer.hpp. 
// Created by brian on 2024-03-26.
//

#ifndef TIMER_HPP
#define TIMER_HPP

#include <chrono>
#include <string>
#include <string_view>

namespace hd ::type {
template <typename T = std::chrono::milliseconds>
struct Timer {
public:
  [[maybe_unused]]
  Timer(std::string& msg_buf)
    : elapsed(mElapsed),
      msg_buf(msg_buf),
      task_name("计时任务"),
      start(std::chrono::high_resolution_clock::now()) {}

  [[maybe_unused]]
  Timer(const std::string_view task, std::string& msg_buf)
    : elapsed(mElapsed),
      msg_buf(msg_buf),
      task_name(task), start(std::chrono::high_resolution_clock::now()) {}

  [[maybe_unused]]
  Timer(size_t& _elapsed, std::string& msg_buf)
    : elapsed(_elapsed),
      msg_buf(msg_buf),
      task_name(""), start(std::chrono::high_resolution_clock::now()) {}

  [[maybe_unused]]
  Timer(size_t& _elapsed, const std::string_view task, std::string& msg_buf)
    : elapsed(_elapsed),
      msg_buf(msg_buf),
      task_name(task), start(std::chrono::high_resolution_clock::now()) {}

  [[maybe_unused]]
  Timer(const std::string_view task)
    : elapsed(mElapsed), msg_buf(msg_buf),
      task_name(task), start(std::chrono::high_resolution_clock::now()) {}

  ~Timer() {
    auto const end = std::chrono::high_resolution_clock::now();
    auto __elapsed = std::chrono::duration_cast<T>(end - start).count();
    std::string _msg;
    auto m = std::to_string(__elapsed);
    _msg.append(task_name)
        .append(m).append(unit())
        .append(8 - m.size(), ' ');
    if (elapsed == std::numeric_limits<size_t>::max()) {
      ELOG_INFO << _msg;
    } else {
      elapsed = __elapsed;
      msg_buf = _msg;
    }
  }

private:
  size_t& elapsed;
  std::string& msg_buf;
  std::string task_name;
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  size_t mElapsed{std::numeric_limits<size_t>::max()};

  static constexpr const char* unit() {
    if constexpr (std::is_same_v<T, std::chrono::seconds>) return "s";
    if constexpr (std::is_same_v<T, std::chrono::milliseconds>) return "ms";
    if constexpr (std::is_same_v<T, std::chrono::microseconds>) return "us";
    if constexpr (std::is_same_v<T, std::chrono::nanoseconds>) return "ns";
    return {};
  }
};
} // type

#endif //TIMER_HPP
