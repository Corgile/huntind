//
// hound-torch / hd_timer.hpp. 
// Created by brian on 2024-03-25.
//

#ifndef HD_TIMER_HPP
#define HD_TIMER_HPP

#include <chrono>
#include <string>
#include <string_view>

namespace hd::type {
template<typename T = std::chrono::milliseconds>
class Timer {
public:
  Timer(std::string& msg_buf)
    : task_name(""), msg_buf(msg_buf), start(std::chrono::high_resolution_clock::now()) {
    // Constructor body is empty since initialization is done through the member initializer list.
  }

  Timer(std::string_view task, std::string& msg_buf)
    : task_name(task), msg_buf(msg_buf), start(std::chrono::high_resolution_clock::now()) {
    // Constructor body is empty for the same reason.
  }

  ~Timer() {
    auto end = std::chrono::high_resolution_clock::now();
    elapsed_time = end - start;
    auto duration = std::chrono::duration_cast<T>(elapsed_time).count();

    // Append the timing information to the external msg_buf string.
    msg_buf.append(task_name)
      .append(" 耗时: ")
      .append(std::to_string(duration))
      .append(unit());
  }

private:
  std::string task_name;
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  std::string& msg_buf; // Holds a reference to an external string to modify.
  std::chrono::duration<double> elapsed_time = std::chrono::duration<double>::zero();

  static constexpr const char* unit() {
    if constexpr (std::is_same_v<T, std::chrono::seconds>)        return " s";
    if constexpr (std::is_same_v<T, std::chrono::milliseconds>)   return " ms";
    if constexpr (std::is_same_v<T, std::chrono::microseconds>)   return " us";
    return {};
  }
};
}

#endif //HD_TIMER_HPP
