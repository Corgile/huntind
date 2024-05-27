//
// hound-torch / task_executor.cpp
// Created by brian on 2024 May 24.
//

#include "hound/task_executor.hpp"
#include "hound/common/global.hpp"

TaskExecutor::~TaskExecutor() {
  mIsRunning = false;
  mCondition.notify_all();
  if (mThread.joinable()) {
    mThread.join();
  }
}

void TaskExecutor::AddTask(std::function<void()> task) {
  int cpu_id = next_cpu_id.fetch_add(1) % hd::global::opt.num_cpus;
  {
    std::lock_guard lock(mMutex);
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
      std::unique_lock lock(mMutex);
      mCondition.wait(lock, [this] { return not mTasks.empty() or not mIsRunning; });
      if (not mIsRunning) break;
      std::tie(task_func, cpu_id) = std::move(mTasks.front());
      mTasks.pop();
    }
    auto task_future = std::async(std::launch::async, [cpu_id, task_=std::move(task_func), this] {
      SetThreadAffinity(cpu_id);
      task_();
    });
    {
      std::scoped_lock<std::mutex> lock(mMutex);
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
  std::lock_guard<std::mutex> lock(mMutex);
  auto new_end = std::ranges::remove_if(mFutures, [](std::future<void>& f) {
    return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
  });
  mFutures.erase(new_end.begin(), new_end.end());
}
