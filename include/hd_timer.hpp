//
// hound-torch / hd_timer.hpp.
// Created by brian on 2024-03-25.
//

#ifndef HD_TIMER_HPP
#define HD_TIMER_HPP
#include <chrono>
#include <string>
#include <string_view>

namespace xhl {
template <typename T = std::chrono::milliseconds>
struct Timer {
  Timer(std::string& _msg_buf)
    : task_name{"任务"}
      , msg_buf(std::move(_msg_buf))
      , start(std::chrono::high_resolution_clock::now()) {}

  Timer(const std::string_view _task, std::string& _msg_buf)
    : task_name(_task)
      , msg_buf(std::move(_msg_buf))
      , start(std::chrono::high_resolution_clock::now()) {}

  ~Timer() {
    auto end = std::chrono::high_resolution_clock::now();
    elapsed_time = end - start;
    auto duration = std::chrono::duration_cast<T>(elapsed_time).count();
    msg_buf.append(task_name).append(" 耗时: ").append(std::to_string(duration)).append(unit());
  }

private:
  std::string task_name;
  std::string&& msg_buf;
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  std::chrono::duration<double> elapsed_time = std::chrono::duration<double>::zero();

  static constexpr std::string unit() {
    if constexpr (std::is_same_v<T, std::chrono::seconds>) return " s";
    if constexpr (std::is_same_v<T, std::chrono::milliseconds>) return " ms";
    if constexpr (std::is_same_v<T, std::chrono::microseconds>) return " us";
    return {};
  }
};
}

#endif //HD_TIMER_HPP
