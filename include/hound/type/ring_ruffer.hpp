//
// Created by brian on 12/4/23.
//

#ifndef HOUND_RING_BUFFER_HPP
#define HOUND_RING_BUFFER_HPP

#include <vector>
#include <list>
#include <atomic>
#include <mutex>
#include <pcap/pcap.h>

namespace hd {
namespace entity {
constexpr int BUFFER_SIZE = 8192;

struct RingBuffer {
  std::pair<pcap_pkthdr, std::vector<u_char>> buffer[BUFFER_SIZE];
  std::atomic<int> head{0};
  std::atomic<int> tail{0};
  std::atomic<bool> full_{false};

  bool full() const { return full_;}

  void push(const pcap_pkthdr* header, const u_char* packet, int32_t len) {
    /// len: min(pad, cap_len)
    int current_tail = tail.load(std::memory_order_relaxed);
    int next_tail = (current_tail + 1) % BUFFER_SIZE;
    while (next_tail == head.load(std::memory_order_acquire)) {
      // 缓冲区满，可以选择覆盖最旧数据或其他策略
      head.store((head.load(std::memory_order_relaxed) + 1) % BUFFER_SIZE, std::memory_order_release);
    }
    buffer[current_tail] = std::make_pair(*header, std::vector<u_char>(packet, packet + len));
    tail.store(next_tail, std::memory_order_release);
  }

  bool pop(pcap_pkthdr& header, std::vector<u_char>& packet) {
    int current_head = head.load(std::memory_order_relaxed);
    if (current_head == tail.load(std::memory_order_acquire)) {
      return false; // 缓冲区空
    }
    auto& pair = buffer[current_head];
    header = pair.first;
    packet = pair.second;
    head.store((current_head + 1) % BUFFER_SIZE, std::memory_order_release);
    return true;
  }
};

class BufferManager {
public:
  void push(const pcap_pkthdr* header, const u_char* packet, int32_t len) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (currentBuffer().full()) {
      // 当前缓冲区已满，创建一个新的缓冲区
      buffers_.emplace_back();
    }
    currentBuffer().push(header, packet, len);
  }

  bool pop(pcap_pkthdr& header, std::vector<u_char>& packet) {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!buffers_.empty()) {
      if (buffers_.front().pop(header, packet)) {
        return true;
      } else {
        // 当前缓冲区已空，移除它
        buffers_.pop_front();
      }
    }
    return false; // 所有缓冲区都空
  }

private:
  std::list<RingBuffer> buffers_;
  std::mutex mutex_;

  RingBuffer& currentBuffer() {
    return buffers_.back();
  }
};

} // entity
} // hd

#endif //HOUND_RING_BUFFER_HPP
