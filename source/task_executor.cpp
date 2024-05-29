//
// hound-torch / task_executor.cpp
// Created by brian on 2024 May 24.
//

#include "hound/task_executor.hpp"
#include "hound/common/global.hpp"

TaskExecutor::~TaskExecutor() {
  {
    std::lock_guard<std::mutex> lock(mtxTaskQue);
    mIsRunning = false;
    mCondition.notify_all();
  }
  if (mThread.joinable()) mThread.join();
  CleanFutures();
}

void TaskExecutor::AddTask(std::function<void()> const& task) {
  auto cpu_id = next_cpu_id.fetch_add(1) % hd::global::opt.num_cpus;
  {
    std::scoped_lock<std::mutex> lock(mtxTaskQue);
    mTasks.emplace(task, cpu_id);
  }
  mCondition.notify_one();
  CleanFutures();
}

void TaskExecutor::Run() {
  while (mIsRunning) {
    std::function<void()> task_func;
    int cpu_id;
    {
      std::unique_lock<std::mutex> lock(mtxTaskQue);
      mCondition.wait(lock, [this] { return !mTasks.empty() || !mIsRunning; });
      if (!mIsRunning && mTasks.empty()) {
        break;
      }
      std::tie(task_func, cpu_id) = std::move(mTasks.front());
      mTasks.pop();
    }
    auto task_future = std::async(std::launch::async, [cpu_id, task_ = std::move(task_func)] {
      TaskExecutor::SetThreadAffinity(cpu_id);
      task_();
    });
    {
      std::lock_guard<std::mutex> lock(mtxFutureQue);
      mFutures.emplace_back(std::move(task_future));
    }
  }
  CleanFutures();
}

void TaskExecutor::SetThreadAffinity(int cpu_id) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu_id, &cpuset);
  pthread_t current_thread = pthread_self();
  pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}

void TaskExecutor::CleanFutures() {
  std::lock_guard<std::mutex> lock(mtxFutureQue);
  auto removeItems = std::ranges::remove_if(mFutures, [](std::future<void>& f) {
    return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
  });
  mFutures.erase(removeItems.begin(), removeItems.end());
}
