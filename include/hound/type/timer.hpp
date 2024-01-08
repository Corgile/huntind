//
// Created by brian on 11/23/23.
//
#ifndef HOUND_TIMER_HPP
#define HOUND_TIMER_HPP

#include <chrono>

namespace hd::type {
class [[nodiscard]] Timer {
public:
  Timer() = delete;

  explicit Timer(double &elapsed1, double &elapsed2) :
    m_elapsed1(elapsed1), m_elapsed2(elapsed2) {
    m_elapsed1 = 0;
    m_elapsed2 = 0;
  }

  ~Timer() = default;

  void start() {
    start_time = std::chrono::high_resolution_clock::now();
  }

  void stop1() const {
    auto const elapsed1 = std::chrono::high_resolution_clock::now() - start_time;
    m_elapsed1 = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed1).count();
  }


void stop2() const {
    auto const elapsed2 = std::chrono::high_resolution_clock::now() - start_time;
    m_elapsed2 = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed2).count();
  }

private:
  double& m_elapsed1;
  double& m_elapsed2;
  std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
  std::chrono::duration<double> elapsed_time = std::chrono::duration<double>::zero();
};

} // entity

#endif //HOUND_TIMER_HPP
