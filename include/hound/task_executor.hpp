//
// hound-torch / task_executor.hpp
// Created by brian on 2024 May 24.
//

#ifndef TASK_EXECUTOR_HPP
#define TASK_EXECUTOR_HPP

#include <future>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "hound/interruptible_sleep.hpp"

class TaskExecutor {
public:
  TaskExecutor();

  ~TaskExecutor();

  void AddTask(std::function<void()> const& task);

  void Run();

private:
  static void SetThreadAffinity(int cpu_id);

  void CleanFutures();

  std::thread mThread;
  std::thread mCleanFutureThread;
  std::mutex mtxTaskQue;
  std::mutex mtxFutureQue;
  InterruptibleSleep mSleeper;
  std::condition_variable mCondition;
  std::atomic_bool mIsRunning;
  std::queue<std::pair<std::function<void()>, int>> mTasks;
  std::vector<std::future<void>> mFutures; // Store futures of running tasks
  std::atomic<int> next_cpu_id{8}; // Next CPU ID to assign
};

#endif //TASK_EXECUTOR_HPP
