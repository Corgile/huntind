//
// hound-torch / timer.hpp. 
// Created by brian on 2024-03-26.
//

#ifndef TIMER_HPP
#define TIMER_HPP
#include <chrono>
#include <string>
#include <string_view>

namespace hd :: type {
template <typename T = std::chrono::milliseconds>
struct Timer {
public:
  Timer(std::string& msg_buf)
    : elapsed(mElapsed),
      msg_buf(msg_buf),
      task_name("计时任务"),
      start(std::chrono::high_resolution_clock::now()) {}

  Timer(const std::string_view task, std::string& msg_buf)
    : elapsed(mElapsed),
      msg_buf(msg_buf),
      task_name(task), start(std::chrono::high_resolution_clock::now()) {}

  Timer(long& _elapsed, std::string& msg_buf)
    : elapsed(_elapsed),
      msg_buf(msg_buf),
      task_name("计时任务"), start(std::chrono::high_resolution_clock::now()) {}

  Timer(long& _elapsed, const std::string_view task, std::string& msg_buf)
    : elapsed(_elapsed),
      msg_buf(msg_buf),
      task_name(task), start(std::chrono::high_resolution_clock::now()) {}

  ~Timer() {
    auto const end = std::chrono::high_resolution_clock::now();
    elapsed = std::chrono::duration_cast<T>(end - start).count();
    msg_buf.append(task_name)
           .append(" 耗时: ")
           .append(std::to_string(elapsed))
           .append(unit());
  }

private:
  long& elapsed;
  std::string& msg_buf;
  std::string task_name;
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  std::chrono::duration<double> elapsed_time = std::chrono::duration<double>::zero();
  long mElapsed{};

  static constexpr const char* unit() {
    if constexpr (std::is_same_v<T, std::chrono::seconds>) return " s";
    if constexpr (std::is_same_v<T, std::chrono::milliseconds>) return " ms";
    if constexpr (std::is_same_v<T, std::chrono::microseconds>) return " us";
    return {};
  }
};
} // type

#endif //TIMER_HPP
