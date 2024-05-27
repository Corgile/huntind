//
// hound-torch / double_buffered_queue.hpp
// Created by brian on 2024-05-26.
//

#ifndef HOUND_TORCH_DOUBLE_BUFFERED_QUEUE_HPP
#define HOUND_TORCH_DOUBLE_BUFFERED_QUEUE_HPP

#include <atomic>

template<typename Data>
class DoubleBufferQueue {
  using container = moodycamel::ConcurrentQueue<Data>;
public:
  DoubleBufferQueue() : current(0) {}

  // Add item to the active queue
  void enqueue(Data& item) {
    queues[current.load()].enqueue(item);
  }

  /// Add item to the active queue
  void enqueue(Data&& item) {
    queues[current.load()].enqueue(item);
  }

  /// Read the active queue and return the queue to be processed
  std::shared_ptr<container> read() {
    int currentQueue = current.load();
    int nextQueue = (currentQueue + 1) & 0x1;
    current.store(nextQueue);
    /// Return the queue that is now inactive for processing
    std::scoped_lock read_lock(read_);
    return std::make_shared<container>(std::move(queues[currentQueue]));
  }

  size_t size() const {
    return queues[0].size_approx() + queues[1].size_approx();
  }

private:
  container queues[2];
  std::mutex read_;
  std::atomic<int> current;
};


#endif //HOUND_TORCH_DOUBLE_BUFFERED_QUEUE_HPP
