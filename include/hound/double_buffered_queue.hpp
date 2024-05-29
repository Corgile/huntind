//
// hound-torch / double_buffered_queue.hpp
// Created by brian on 2024-05-26.
//

#ifndef HOUND_TORCH_DOUBLE_BUFFERED_QUEUE_HPP
#define HOUND_TORCH_DOUBLE_BUFFERED_QUEUE_HPP

#include <atomic>

template<typename Data, typename container = moodycamel::ConcurrentQueue<Data>>
class DoubleBufferQueue {
public:
  DoubleBufferQueue() : current(0) {}

  // Add item to the active queue
  void enqueue(Data& item) {
    if constexpr (std::is_same_v<container, std::vector<Data>>) {
      queues[current.load()].emplace_back(std::forward<Data>(item));
    } else queues[current.load()].enqueue(item);
  }

  /// Add item to the active queue
  void enqueue(Data&& item) {
    if constexpr (std::is_same_v<container, std::vector<Data>>) {
      queues[current.load()].emplace_back(std::forward<Data>(item));
    } else queues[current.load()].enqueue(item);
  }

  /// Read the active queue and return the queue to be processed
  std::shared_ptr<container> read() {
    int currentQueue = current.load();
    int nextQueue = (currentQueue + 1) & 0x1;
    current.store(nextQueue);
    /// Return the queue that is now inactive for processing
    container buffer;
    {
      std::scoped_lock read_lock(read_);
      buffer.swap(queues[currentQueue]);
    }
    return std::make_shared<container>(std::move(buffer));
  }

  size_t size() const {
    if constexpr (std::is_same_v<container, std::vector<Data>>) {
      return queues[0].size() + queues[1].size();
    }
    else return queues[0].size_approx() + queues[1].size_approx();
  }

private:
  container queues[2];
  std::mutex read_;
  std::atomic<int> current;
};


#endif //HOUND_TORCH_DOUBLE_BUFFERED_QUEUE_HPP
