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

class TaskExecutor {
public:
  TaskExecutor() : mIsRunning(true) {
    mThread = std::thread(&TaskExecutor::Run, this);
  }

  ~TaskExecutor();

  void AddTask(std::function<void()> task);

private:
  void Run();

  std::queue<std::function<void()>> mTasks;
  std::thread mThread;
  std::mutex mMutex;
  std::condition_variable mCondition;
  bool mIsRunning;
};

#endif //TASK_EXECUTOR_HPP
