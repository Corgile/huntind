//
// hound-torch / task_executor.cpp
// Created by brian on 2024 May 24.
//

#include "hound/task_executor.hpp"

TaskExecutor::~TaskExecutor() {
  {
    std::lock_guard lock(mMutex);
    mIsRunning = false;
  }
  mCondition.notify_all();
  if (mThread.joinable()) {
    mThread.join();
  }
}

void TaskExecutor::AddTask(std::function<void()> task) {
  {
    std::lock_guard lock(mMutex);
    mTasks.push(task);
  }
  mCondition.notify_one();
}

void TaskExecutor::Run() {
  while (mIsRunning) {
    std::function<void()> task;
    {
      std::unique_lock lock(mMutex);
      mCondition.wait(lock, [this] { return not mTasks.empty() or not mIsRunning; });
      task = std::move(mTasks.front());
      mTasks.pop();
    }
    try {
      task();
    } catch (...) {}
  }
}
